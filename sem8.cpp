#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <barrier>
#include <random>
#include <chrono>
#include <queue>
#include <algorithm>
#include <execution>
#include <numeric>
#include <fstream>
#include <iomanip>
#include <concurrent_queue.h>  

using namespace std;
using namespace std::chrono;


struct TelemetryData {
    int sensor_id;
    double temperature;
    double pressure;
    double vibration;
    long long timestamp_ms;

    void print() const {
        cout << "Sensor[" << sensor_id << "] T:" << temperature
            << " P:" << pressure << " V:" << vibration << endl;
    }

    bool operator<(const TelemetryData& other) const {
        return temperature < other.temperature;
    }
};


template<typename T>
class ThreadSafeQueue {
private:
    queue<T> q;
    mutable mutex mtx;
    condition_variable cv;
    atomic<bool> finished{ false };

public:
    void push(T value) {
        {
            lock_guard<mutex> lock(mtx);
            q.push(move(value));
        }
        cv.notify_one();
    }

    bool pop(T& value) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this] { return !q.empty() || finished; });

        if (finished && q.empty()) return false;

        value = move(q.front());
        q.pop();
        return true;
    }

    void set_finished() {
        finished = true;
        cv.notify_all();
    }

    size_t size() const {
        lock_guard<mutex> lock(mtx);
        return q.size();
    }
};


class SensorSimulator {
private:
    int num_sensors;
    ThreadSafeQueue<TelemetryData>& input_queue;
    atomic<int>& active_sensors;
    random_device rd;
    mt19937 gen;
    uniform_real_distribution<> temp_dist;
    uniform_real_distribution<> pressure_dist;
    uniform_real_distribution<> vib_dist;

public:
    SensorSimulator(int sensors, ThreadSafeQueue<TelemetryData>& queue, atomic<int>& active)
        : num_sensors(sensors), input_queue(queue), active_sensors(active),
        gen(rd()), temp_dist(20.0, 100.0), pressure_dist(0.8, 1.2), vib_dist(0.0, 5.0) {
    }

    void run(int thread_id, int sensors_per_thread) {
        int start = thread_id * sensors_per_thread;
        int end = min(start + sensors_per_thread, num_sensors);

        for (int i = 0; i < 10; i++) {  
            for (int s = start; s < end; s++) {
                TelemetryData data;
                data.sensor_id = s;
                data.temperature = temp_dist(gen);
                data.pressure = pressure_dist(gen);
                data.vibration = vib_dist(gen);
                data.timestamp_ms = duration_cast<milliseconds>(
                    system_clock::now().time_since_epoch()
                ).count();

                input_queue.push(data);

                cout << "[Sensor Thread " << thread_id << "] Sensor[" << s
                    << "] sent data" << endl;

                this_thread::sleep_for(milliseconds(10));
            }
        }

        active_sensors--;
        cout << "[Sensor Thread " << thread_id << "] finished. Active: "
            << active_sensors.load() << endl;
    }
};


class ProcessingPool {
private:
    ThreadSafeQueue<TelemetryData>& input_queue;
    vector<TelemetryData> processed_data;
    shared_mutex data_mutex;
    atomic<int> processed_count{ 0 };
    barrier<> sync_barrier;

public:
    ProcessingPool(ThreadSafeQueue<TelemetryData>& queue, int num_threads)
        : input_queue(queue), sync_barrier(num_threads) {
    }

    void process_batch(const vector<TelemetryData>& batch, int thread_id) {
        thread_local vector<double> local_temps;
        local_temps.clear();


        double avg_temp = transform_reduce(
            execution::par_unseq,
            batch.begin(), batch.end(),
            0.0,
            [](double a, double b) { return a + b; },
            [](const TelemetryData& d) { return d.temperature; }
        ) / batch.size();

        double max_pressure = transform_reduce(
            execution::par,
            batch.begin(), batch.end(),
            0.0,
            [](double a, double b) { return max(a, b); },
            [](const TelemetryData& d) { return d.pressure; }
        );

        vector<TelemetryData> sorted_batch = batch;
        sort(execution::par, sorted_batch.begin(), sorted_batch.end());

        vector<double> temps(batch.size());
        transform(execution::par, batch.begin(), batch.end(), temps.begin(),
            [](const TelemetryData& d) { return d.temperature; });

        vector<double> cumulative_temps(temps.size());
        inclusive_scan(execution::par, temps.begin(), temps.end(),
            cumulative_temps.begin(), plus<double>());

        double local_avg_vib = 0;
        for (const auto& d : batch) {
            local_avg_vib += d.vibration;
            local_temps.push_back(d.temperature);
        }
        local_avg_vib /= batch.size();

        sync_barrier.arrive_and_wait();

        {
            unique_lock<shared_mutex> lock(data_mutex);
            for (const auto& d : batch) {
                processed_data.push_back(d);
            }
            processed_count += batch.size();
        }

        cout << "[Processing Thread " << thread_id << "] Processed " << batch.size()
            << " items. Avg Temp: " << fixed << setprecision(2) << avg_temp
            << ", Max Pressure: " << max_pressure
            << ", Local Avg Vib: " << local_avg_vib << endl;
    }

    void run(int thread_id) {
        vector<TelemetryData> local_batch;
        const size_t BATCH_SIZE = 100;

        while (true) {
            TelemetryData data;
            if (!input_queue.pop(data)) {
                break;
            }

            local_batch.push_back(data);

            if (local_batch.size() >= BATCH_SIZE) {
                process_batch(local_batch, thread_id);
                local_batch.clear();
                this_thread::yield();
            }
        }

        if (!local_batch.empty()) {
            process_batch(local_batch, thread_id);
        }

        cout << "[Processing Thread " << thread_id << "] finished. Total processed: "
            << processed_count.load() << endl;
    }

    vector<TelemetryData> get_results() {
        shared_lock<shared_mutex> lock(data_mutex);
        return processed_data;
    }
};

class ResultAggregator {
private:
    vector<TelemetryData> results;
    mutex result_mutex;
    atomic<int> total_processed{ 0 };

public:
    void add_results(const vector<TelemetryData>& data) {
        lock_guard<mutex> lock(result_mutex);
        for (const auto& d : data) {
            results.push_back(d);
        }
        total_processed += data.size();
    }

    void print_statistics() {
        if (results.empty()) return;

        double avg_temp = transform_reduce(
            execution::par_unseq,
            results.begin(), results.end(),
            0.0,
            plus<double>(),
            [](const TelemetryData& d) { return d.temperature; }
        ) / results.size();

        double max_vib = transform_reduce(
            execution::par,
            results.begin(), results.end(),
            0.0,
            [](double a, double b) { return max(a, b); },
            [](const TelemetryData& d) { return d.vibration; }
        );

        vector<TelemetryData> sorted = results;
        sort(execution::par, sorted.begin(), sorted.end(),
            [](const TelemetryData& a, const TelemetryData& b) {
                return a.temperature > b.temperature;
            });

        cout << "\n========== FINAL STATISTICS ==========\n";
        cout << "Total processed: " << total_processed.load() << endl;
        cout << "Average temperature: " << fixed << setprecision(2) << avg_temp << endl;
        cout << "Maximum vibration: " << max_vib << endl;
        cout << "\nTop 5 highest temperatures:\n";

        for (int i = 0; i < min(5, (int)sorted.size()); i++) {
            cout << "  " << i + 1 << ". Sensor[" << sorted[i].sensor_id
                << "] Temp: " << sorted[i].temperature << endl;
        }
    }

    void save_to_file(const string& filename) {
        ofstream file(filename);
        if (file.is_open()) {
            file << "Sensor_ID,Temperature,Pressure,Vibration,Timestamp\n";
            for (const auto& d : results) {
                file << d.sensor_id << ","
                    << d.temperature << ","
                    << d.pressure << ","
                    << d.vibration << ","
                    << d.timestamp_ms << "\n";
            }
            file.close();
            cout << "\nResults saved to: " << filename << endl;
        }
    }
};


class TelemetrySystem {
private:
    static constexpr int NUM_SENSORS = 100;
    static constexpr int NUM_SENSOR_THREADS = 4;
    static constexpr int NUM_PROCESSING_THREADS = 4;

    ThreadSafeQueue<TelemetryData> input_queue;
    ProcessingPool processing_pool;
    ResultAggregator aggregator;

    atomic<int> active_sensor_threads;

public:
    TelemetrySystem()
        : processing_pool(input_queue, NUM_PROCESSING_THREADS),
        active_sensor_threads(NUM_SENSOR_THREADS) {
    }

    void run() {
        auto start_time = steady_clock::now();

        cout << "========================================\n";
        cout << "TELEMETRY ANALYSIS SYSTEM\n";
        cout << "========================================\n";
        cout << "Sensors: " << NUM_SENSORS << "\n";
        cout << "Sensor threads: " << NUM_SENSOR_THREADS << "\n";
        cout << "Processing threads: " << NUM_PROCESSING_THREADS << "\n";
        cout << "========================================\n\n";

        vector<thread> sensor_threads;
        int sensors_per_thread = NUM_SENSORS / NUM_SENSOR_THREADS;

        for (int i = 0; i < NUM_SENSOR_THREADS; i++) {
            SensorSimulator simulator(NUM_SENSORS, input_queue, active_sensor_threads);
            sensor_threads.emplace_back([&simulator, i, sensors_per_thread]() {
                simulator.run(i, sensors_per_thread);
                });
        }

        vector<thread> processing_threads;
        for (int i = 0; i < NUM_PROCESSING_THREADS; i++) {
            processing_threads.emplace_back([this, i]() {
                processing_pool.run(i);
                });
        }

        for (auto& t : sensor_threads) {
            t.join();
        }

        input_queue.set_finished();

        for (auto& t : processing_threads) {
            t.join();
        }

        auto end_time = steady_clock::now();
        auto duration = duration_cast<milliseconds>(end_time - start_time);

        auto results = processing_pool.get_results();
        aggregator.add_results(results);
        aggregator.print_statistics();
        aggregator.save_to_file("telemetry_results.csv");

        cout << "\nTotal execution time: " << duration.count() << " ms\n";
    }
};


int main() {
    TelemetrySystem system;
    system.run();

    cout << "\n========================================\n";
    cout << "System shutdown complete.\n";
    cout << "========================================\n";

    return 0;
}