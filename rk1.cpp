#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <random>
#include <atomic>
#include <semaphore>
#include <vector>
#include <iomanip>

using namespace std;
using namespace std::chrono;


class Warehouse {
private:
    static const int NUM_ZONES = 2;
    static const int TARGET_STOCK = 10;

    vector<int> stock; 
    vector<bool> zone_locked;  
    mutex mtx;
    condition_variable cv;
    atomic<int> total_operations;

    random_device rd;
    mt19937 gen;
    uniform_int_distribution<> check_duration;
    uniform_int_distribution<> add_duration;

public:
    Warehouse() : stock(NUM_ZONES, 5), zone_locked(NUM_ZONES, false),
        total_operations(0), gen(rd()),
        check_duration(500, 1500), add_duration(1000, 2000) {
    }

    void check_operator(int operator_id) {
        while (total_operations < 20) {
            int zone_id = -1;

            {
                lock_guard<mutex> lock(mtx);
                int min_stock = 100;
                for (int i = 0; i < NUM_ZONES; i++) {
                    if (!zone_locked[i] && stock[i] < min_stock) {
                        min_stock = stock[i];
                        zone_id = i;
                    }
                }
            }

            if (zone_id == -1) {
                this_thread::sleep_for(milliseconds(100));
                continue;
            }

            {
                lock_guard<mutex> lock(mtx);
                zone_locked[zone_id] = true;
            }

            int check_time = check_duration(gen);
            cout << "Operator " << operator_id << " (CHECKER) checking zone " << zone_id
                << ", current stock: " << stock[zone_id]
                << " (takes " << check_time / 1000.0 << " sec)" << endl;

            this_thread::sleep_for(milliseconds(check_time));

            if (stock[zone_id] < TARGET_STOCK) {
                int add_time = add_duration(gen);
                cout << "Operator " << operator_id << " (CHECKER) adding stock to zone " << zone_id
                    << " (takes " << add_time / 1000.0 << " sec)" << endl;

                this_thread::sleep_for(milliseconds(add_time));

                {
                    lock_guard<mutex> lock(mtx);
                    stock[zone_id] = min(stock[zone_id] + 3, TARGET_STOCK + 5);
                    total_operations++;
                }
            }

            {
                lock_guard<mutex> lock(mtx);
                zone_locked[zone_id] = false;
                cout << "Operator " << operator_id << " (CHECKER) released zone " << zone_id
                    << ", new stock: " << stock[zone_id] << endl;
            }
            cv.notify_all();

            this_thread::yield();
        }
    }

    void fill_operator(int operator_id) {
        while (total_operations < 20) {
            int zone_id = -1;

            {
                lock_guard<mutex> lock(mtx);
                int max_deficit = -1;
                for (int i = 0; i < NUM_ZONES; i++) {
                    if (!zone_locked[i] && (TARGET_STOCK - stock[i]) > max_deficit) {
                        max_deficit = TARGET_STOCK - stock[i];
                        zone_id = i;
                    }
                }
            }

            if (zone_id == -1 || stock[zone_id] >= TARGET_STOCK) {
                this_thread::sleep_for(milliseconds(100));
                continue;
            }

            {
                lock_guard<mutex> lock(mtx);
                zone_locked[zone_id] = true;
            }

            int add_time = add_duration(gen);
            cout << "Operator " << operator_id << " (FILLER) adding stock to zone " << zone_id
                << " (takes " << add_time / 1000.0 << " sec)" << endl;

            this_thread::sleep_for(milliseconds(add_time));

            {
                lock_guard<mutex> lock(mtx);
                stock[zone_id] = min(stock[zone_id] + 5, TARGET_STOCK + 5);
                total_operations++;
                cout << "Operator " << operator_id << " (FILLER) finished zone " << zone_id
                    << ", new stock: " << stock[zone_id] << endl;
            }

            {
                lock_guard<mutex> lock(mtx);
                zone_locked[zone_id] = false;
            }
            cv.notify_all();

            this_thread::yield();
        }
    }

    void run() {
        vector<thread> operators;

        cout << "Starting warehouse system..." << endl;
        cout << "Zones: " << NUM_ZONES << ", Target stock: " << TARGET_STOCK << endl;
        cout << "Initial stock: ";
        for (int i = 0; i < NUM_ZONES; i++) {
            cout << "Zone " << i << "=" << stock[i] << " ";
        }
        cout << "\n----------------------------------------" << endl;

        operators.emplace_back(&Warehouse::check_operator, this, 1);
        operators.emplace_back(&Warehouse::fill_operator, this, 2);

        for (auto& t : operators) {
            t.join();
        }

        cout << "----------------------------------------" << endl;
        cout << "Final stock: ";
        for (int i = 0; i < NUM_ZONES; i++) {
            cout << "Zone " << i << "=" << stock[i] << " ";
        }
        cout << "\nTotal operations: " << total_operations << endl;
    }
};

void task1_variant3() {
    cout << "\n========== VARIANT 3 - TASK 1 ==========\n";
    cout << "Warehouse with 2 operators (Checker and Filler)\n\n";

    Warehouse warehouse;
    warehouse.run();
}

class ProductionSystem {
private:
    static const int NUM_MATERIALS = 5;

    counting_semaphore<> load_semaphore;
    counting_semaphore<> process_semaphore;
    counting_semaphore<> pack_semaphore;

    atomic<int> completed_products;
    mutex console_mtx;

    random_device rd;
    mt19937 gen;
    uniform_int_distribution<> load_duration;
    uniform_int_distribution<> process_duration;
    uniform_int_distribution<> pack_duration;

public:
    ProductionSystem()
        : load_semaphore(2),   
        process_semaphore(0),   
        pack_semaphore(0),      
        completed_products(0),
        gen(rd()),
        load_duration(500, 1500),
        process_duration(1000, 2000),
        pack_duration(500, 1000) {
    }

    void load_stage(int product_id) {
        {
            lock_guard<mutex> lock(console_mtx);
            cout << "Product " << product_id << ": LOADING started" << endl;
        }

        int duration = load_duration(gen);
        this_thread::sleep_for(milliseconds(duration));

        {
            lock_guard<mutex> lock(console_mtx);
            cout << "Product " << product_id << ": LOADING completed ("
                << duration / 1000.0 << " sec)" << endl;
        }

        process_semaphore.release();
    }

    void process_stage(int product_id) {
        process_semaphore.acquire();

        {
            lock_guard<mutex> lock(console_mtx);
            cout << "Product " << product_id << ": PROCESSING started" << endl;
        }

        int duration = process_duration(gen);
        this_thread::sleep_for(milliseconds(duration));

        {
            lock_guard<mutex> lock(console_mtx);
            cout << "Product " << product_id << ": PROCESSING completed ("
                << duration / 1000.0 << " sec)" << endl;
        }

        pack_semaphore.release();
    }

    void pack_stage(int product_id) {
        pack_semaphore.acquire();

        {
            lock_guard<mutex> lock(console_mtx);
            cout << "Product " << product_id << ": PACKING started" << endl;
        }

        int duration = pack_duration(gen);
        this_thread::sleep_for(milliseconds(duration));

        {
            lock_guard<mutex> lock(console_mtx);
            cout << "Product " << product_id << ": PACKING completed ("
                << duration / 1000.0 << " sec)" << endl;
        }

        completed_products++;
    }

    void run() {
        vector<thread> load_threads;
        vector<thread> process_threads;
        vector<thread> pack_threads;

        cout << "\nStarting production system..." << endl;
        cout << "Stages: LOADING -> PROCESSING -> PACKING" << endl;
        cout << "Number of products: " << NUM_MATERIALS << endl;
        cout << "----------------------------------------" << endl;

        
        for (int i = 0; i < NUM_MATERIALS; i++) {
            load_threads.emplace_back(&ProductionSystem::load_stage, this, i + 1);
            process_threads.emplace_back(&ProductionSystem::process_stage, this, i + 1);
            pack_threads.emplace_back(&ProductionSystem::pack_stage, this, i + 1);
        }

      
        for (auto& t : load_threads) t.join();
        for (auto& t : process_threads) t.join();
        for (auto& t : pack_threads) t.join();

        cout << "----------------------------------------" << endl;
        cout << "All products completed: " << completed_products << "/" << NUM_MATERIALS << endl;
    }
};

void task2_variant3() {
    cout << "\n========== VARIANT 3 - TASK 2 ==========\n";
    cout << "Production System (Loading -> Processing -> Packing)\n\n";

    ProductionSystem system;
    system.run();
}


int main() {
    cout << "========================================\n";
    cout << "RUBEZHNY CONTROL 1 - VARIANT 3\n";
    cout << "========================================\n";

    task1_variant3();
    task2_variant3();

    cout << "\n========================================\n";
    cout << "All tasks completed!\n";
    cout << "========================================\n";

    return 0;
}