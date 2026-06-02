#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <random>
#include <queue>
#include <atomic>
#include <semaphore>
#include <map>
#include <iomanip>

using namespace std;
using namespace std::chrono;

// ЗАДАЧА 1: Resource Pool с приоритетами потоков

template <typename T>
class ResourcePool {
private:
    vector<T> resources;
    counting_semaphore<> semaphore;
    mutex mtx;
    atomic<int> failed_attempts;

public:
    ResourcePool(int count, const vector<T>& res)
        : semaphore(count), failed_attempts(0) {
        resources = res;
    }

    T acquire(int priority, int timeout_ms) {
        auto start = steady_clock::now();

        while (true) {
            if (semaphore.try_acquire()) {
                lock_guard<mutex> lock(mtx);
                if (!resources.empty()) {
                    T res = resources.back();
                    resources.pop_back();
                    cout << "Thread " << this_thread::get_id()
                        << " (priority " << priority << ") acquired resource" << endl;
                    return res;
                }
                else {
                    semaphore.release();
                }
            }

            auto now = steady_clock::now();
            if (duration_cast<milliseconds>(now - start).count() >= timeout_ms) {
                failed_attempts++;
                cout << "Thread " << this_thread::get_id()
                    << " (priority " << priority << ") timeout! Failed attempts: "
                    << failed_attempts.load() << endl;
                throw runtime_error("Timeout acquiring resource");
            }

            this_thread::yield();
        }
    }

    void release(T res) {
        {
            lock_guard<mutex> lock(mtx);
            resources.push_back(res);
        }
        semaphore.release();
        cout << "Thread " << this_thread::get_id() << " released resource" << endl;
    }

    int get_failed_attempts() const { return failed_attempts.load(); }
};

void task1() {
    cout << "\n========== TASK 1: Resource Pool ==========\n";

    vector<string> initial_resources = { "DB_Conn_1", "DB_Conn_2", "DB_Conn_3" };
    ResourcePool<string> pool(3, initial_resources);

    vector<thread> threads;

    for (int i = 0; i < 5; i++) {
        threads.emplace_back([&pool, i]() {
            try {
                int priority = (i % 2 == 0) ? 10 : 1;
                auto res = pool.acquire(priority, 500);
                this_thread::sleep_for(milliseconds(100));
                pool.release(res);
            }
            catch (const exception& e) {
            }
            });
    }

    for (auto& t : threads) {
        t.join();
    }

    cout << "Total failed attempts: " << pool.get_failed_attempts() << endl;
}

// ЗАДАЧА 2: Parking Lot с VIP

class ParkingLot {
private:
    int capacity;
    int occupied;
    counting_semaphore<> semaphore;
    mutex mtx;

public:
    ParkingLot(int cap) : capacity(cap), occupied(0), semaphore(cap) {}

    void park(bool isVIP, int timeout_ms) {
        auto start = steady_clock::now();

        while (true) {
            if (semaphore.try_acquire()) {
                lock_guard<mutex> lock(mtx);
                occupied++;
                cout << "Thread " << this_thread::get_id()
                    << (isVIP ? " VIP" : " REGULAR") << " parked. "
                    << "Occupied: " << occupied << "/" << capacity << endl;
                return;
            }

            auto now = steady_clock::now();
            if (duration_cast<milliseconds>(now - start).count() >= timeout_ms) {
                cout << "Thread " << this_thread::get_id()
                    << (isVIP ? " VIP" : " REGULAR") << " timeout!" << endl;
                return;
            }

            this_thread::yield();
        }
    }

    void leave() {
        semaphore.release();
        {
            lock_guard<mutex> lock(mtx);
            occupied--;
            cout << "Thread " << this_thread::get_id() << " left. "
                << "Occupied: " << occupied << "/" << capacity << endl;
        }
    }
};

void task2() {
    cout << "\n========== TASK 2: Parking Lot ==========\n";

    ParkingLot lot(3);
    vector<thread> threads;

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> vipDist(0, 1);

    for (int i = 0; i < 10; i++) {
        bool isVIP = (vipDist(gen) == 1);
        threads.emplace_back([&lot, isVIP]() {
            lot.park(isVIP, 1000);
            this_thread::sleep_for(milliseconds(500));
            lot.leave();
            });
    }

    for (auto& t : threads) {
        t.join();
    }
}
// ЗАДАЧА 3: Producer-Consumer с несколькими буферами

template <typename T>
class SemaphoreBuffer {
private:
    vector<vector<T>> buffers;
    vector<counting_semaphore<>> empty;
    vector<counting_semaphore<>> full;
    vector<mutex> buffer_mutexes;

public:
    SemaphoreBuffer(int num_buffers, int buf_size)
        : buffers(num_buffers),
        empty(num_buffers, counting_semaphore<>(buf_size)),
        full(num_buffers, counting_semaphore<>(0)),
        buffer_mutexes(num_buffers) {
    }

    void produce(T value, int buffer_index, int timeout_ms) {
        if (empty[buffer_index].try_acquire_for(chrono::milliseconds(timeout_ms))) {
            {
                lock_guard<mutex> lock(buffer_mutexes[buffer_index]);
                buffers[buffer_index].push_back(value);
            }
            full[buffer_index].release();
            cout << "Thread " << this_thread::get_id() << " produced " << value
                << " to buffer " << buffer_index << endl;
        }
        else {
            cout << "Thread " << this_thread::get_id() << " timeout on produce to buffer "
                << buffer_index << endl;
        }
    }

    T consume(int buffer_index, int timeout_ms) {
        if (full[buffer_index].try_acquire_for(chrono::milliseconds(timeout_ms))) {
            T value;
            {
                lock_guard<mutex> lock(buffer_mutexes[buffer_index]);
                value = buffers[buffer_index].back();
                buffers[buffer_index].pop_back();
            }
            empty[buffer_index].release();
            cout << "Thread " << this_thread::get_id() << " consumed " << value
                << " from buffer " << buffer_index << endl;
            return value;
        }
        else {
            cout << "Thread " << this_thread::get_id() << " timeout on consume from buffer "
                << buffer_index << endl;
            return T();
        }
    }
};

void task3() {
    cout << "\n========== TASK 3: Multiple Buffers ==========\n";

    SemaphoreBuffer<int> buffer(3, 5);
    vector<thread> producers;
    vector<thread> consumers;

    for (int i = 0; i < 5; i++) {
        producers.emplace_back([&buffer]() {
            for (int j = 0; j < 3; j++) {
                buffer.produce(j, j % 3, 100);
                this_thread::sleep_for(milliseconds(50));
            }
            });
    }

    for (int i = 0; i < 5; i++) {
        consumers.emplace_back([&buffer]() {
            for (int j = 0; j < 3; j++) {
                buffer.consume(j % 3, 100);
                this_thread::sleep_for(milliseconds(50));
            }
            });
    }

    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();
}

// ЗАДАЧА 4: Printer Queue

struct PrintJob {
    string doc;
    int priority;
    int id;
    bool operator<(const PrintJob& other) const {
        return priority < other.priority;
    }
};

class PrinterQueue {
private:
    int n_printers;
    counting_semaphore<> semaphore;
    mutex mtx;
    priority_queue<PrintJob> job_queue;
    atomic<int> job_counter;

public:
    PrinterQueue(int printers) : n_printers(printers), semaphore(printers), job_counter(0) {}

    void printJob(string doc, int priority, int timeout_ms) {
        int job_id = job_counter++;

        {
            lock_guard<mutex> lock(mtx);
            job_queue.push({ doc, priority, job_id });
        }

        if (semaphore.try_acquire_for(chrono::milliseconds(timeout_ms))) {
            PrintJob job;
            {
                lock_guard<mutex> lock(mtx);
                if (!job_queue.empty() && job_queue.top().id == job_id) {
                    job = job_queue.top();
                    job_queue.pop();
                }
                else {
                    semaphore.release();
                    return;
                }
            }

            cout << "Thread " << this_thread::get_id() << " printing job " << job_id
                << " (priority " << priority << "): " << doc << endl;

            this_thread::sleep_for(milliseconds(200));

            cout << "Thread " << this_thread::get_id() << " completed job " << job_id << endl;
            semaphore.release();
        }
        else {
            cout << "Thread " << this_thread::get_id() << " timeout for job " << job_id << endl;
        }
    }
};

void task4() {
    cout << "\n========== TASK 4: Printer Queue ==========\n";

    PrinterQueue pq(2);
    vector<thread> threads;

    vector<pair<string, int>> jobs = {
        {"Report.pdf", 1},
        {"Urgent.doc", 10},
        {"Invoice.pdf", 5},
        {"Secret.docx", 8},
        {"Notes.txt", 2}
    };

    for (const auto& job : jobs) {
        threads.emplace_back([&pq, job]() {
            pq.printJob(job.first, job.second, 500);
            });
    }

    for (auto& t : threads) {
        t.join();
    }
}

// ЗАДАЧА 5: Task Scheduler

struct Task {
    int id;
    int required_slots;
    int duration_ms;
    int priority;

    bool operator<(const Task& other) const {
        return priority < other.priority;
    }
};

class TaskScheduler {
private:
    priority_queue<Task> task_queue;
    counting_semaphore<> resource_semaphore;
    mutex queue_mutex;
    atomic<int> completed_tasks;

public:
    TaskScheduler(int slots) : resource_semaphore(slots), completed_tasks(0) {}

    void submit(Task task) {
        lock_guard<mutex> lock(queue_mutex);
        task_queue.push(task);
        cout << "Task " << task.id << " submitted (priority " << task.priority
            << ", needs " << task.required_slots << " slots)" << endl;
    }

    inline void execute_task(Task& task) {
        cout << "Thread " << this_thread::get_id() << " executing task " << task.id
            << " (using " << task.required_slots << " slots)" << endl;
        this_thread::sleep_for(milliseconds(task.duration_ms));
        completed_tasks++;
        cout << "Thread " << this_thread::get_id() << " completed task " << task.id << endl;
    }

    void worker() {
        while (completed_tasks.load() < 10) {
            Task task;
            bool has_task = false;
            {
                lock_guard<mutex> lock(queue_mutex);
                if (!task_queue.empty()) {
                    task = task_queue.top();
                    task_queue.pop();
                    has_task = true;
                }
            }

            if (has_task) {
                if (resource_semaphore.try_acquire_for(chrono::milliseconds(500))) {
                    execute_task(task);
                    resource_semaphore.release(task.required_slots);
                }
                else {
                    lock_guard<mutex> lock(queue_mutex);
                    task_queue.push(task);
                    this_thread::yield();
                }
            }
            else {
                this_thread::sleep_for(milliseconds(10));
            }
        }
    }

    int get_completed() const { return completed_tasks.load(); }
};

void task5() {
    cout << "\n========== TASK 5: Task Scheduler ==========\n";

    TaskScheduler scheduler(4);
    vector<thread> workers;

    for (int i = 0; i < 3; i++) {
        workers.emplace_back([&scheduler]() { scheduler.worker(); });
    }

    vector<Task> tasks = {
        {1, 2, 300, 5},
        {2, 1, 200, 8},
        {3, 3, 400, 3},
        {4, 2, 250, 10},
        {5, 1, 150, 7},
        {6, 4, 500, 1},
        {7, 2, 300, 6}
    };

    for (const auto& task : tasks) {
        scheduler.submit(task);
        this_thread::sleep_for(milliseconds(50));
    }

    this_thread::sleep_for(milliseconds(4000));

    for (auto& t : workers) {
        t.join();
    }

    cout << "Completed tasks: " << scheduler.get_completed() << endl;
}

// ЗАДАЧА 6: Download Manager (ИСПРАВЛЕНА)

struct FileChunk {
    int chunk_id;
    int file_id;
    size_t size;
};

class FileDownload {
public:
    int file_id;
    vector<FileChunk> chunks;
    atomic<int> downloaded_chunks;

    FileDownload(int id, int num_chunks) : file_id(id), downloaded_chunks(0) {
        for (int i = 0; i < num_chunks; i++) {
            chunks.push_back({ i, id, 1024 });
        }
    }

    FileDownload(const FileDownload&) = delete;
    FileDownload& operator=(const FileDownload&) = delete;

    FileDownload(FileDownload&& other) noexcept
        : file_id(other.file_id),
        chunks(std::move(other.chunks)),
        downloaded_chunks(other.downloaded_chunks.load()) {
    }

    FileDownload& operator=(FileDownload&& other) noexcept {
        if (this != &other) {
            file_id = other.file_id;
            chunks = std::move(other.chunks);
            downloaded_chunks = other.downloaded_chunks.load();
        }
        return *this;
    }

    bool is_complete() const {
        return downloaded_chunks.load() >= (int)chunks.size();
    }

    void mark_chunk_downloaded() {
        downloaded_chunks++;
    }
};

class DownloadManager {
private:
    queue<FileChunk> download_queue;
    counting_semaphore<> active_downloads;
    counting_semaphore<> chunk_downloads;
    mutex queue_mutex;
    atomic<int> completed_files;
    vector<FileDownload> files;
    mutex files_mutex;

public:
    DownloadManager(int max_files, int max_chunks)
        : active_downloads(max_files), chunk_downloads(max_chunks), completed_files(0) {
    }

    void add_file(FileDownload&& file) {
        {
            lock_guard<mutex> lock(files_mutex);
            files.push_back(std::move(file));
        }

        FileDownload& added_file = files.back();
        for (const auto& chunk : added_file.chunks) {
            lock_guard<mutex> lock(queue_mutex);
            download_queue.push(chunk);
        }

        cout << "File " << added_file.file_id << " added with " << added_file.chunks.size() << " chunks" << endl;
    }

    inline void process_chunk(FileChunk chunk) {
        cout << "Thread " << this_thread::get_id() << " downloading file " << chunk.file_id
            << " chunk " << chunk.chunk_id << " (" << chunk.size << " bytes)" << endl;

        this_thread::sleep_for(milliseconds(50));

        lock_guard<mutex> lock(files_mutex);
        for (auto& file : files) {
            if (file.file_id == chunk.file_id) {
                file.mark_chunk_downloaded();
                if (file.is_complete()) {
                    completed_files++;
                    cout << "File " << chunk.file_id << " complete! ("
                        << completed_files.load() << " files done)" << endl;
                }
                break;
            }
        }
    }

    void download_worker() {
        while (completed_files.load() < 3) {
            FileChunk chunk;
            bool has_chunk = false;
            {
                lock_guard<mutex> lock(queue_mutex);
                if (!download_queue.empty()) {
                    chunk = download_queue.front();
                    download_queue.pop();
                    has_chunk = true;
                }
            }

            if (has_chunk) {
                if (active_downloads.try_acquire_for(chrono::milliseconds(100))) {
                    if (chunk_downloads.try_acquire_for(chrono::milliseconds(100))) {
                        process_chunk(chunk);
                        chunk_downloads.release();
                    }
                    active_downloads.release();
                }
                else {
                    lock_guard<mutex> lock(queue_mutex);
                    download_queue.push(chunk);
                    this_thread::yield();
                }
            }
            else {
                this_thread::sleep_for(milliseconds(10));
            }
        }
    }

    int get_completed_files() const { return completed_files.load(); }
};

void task6() {
    cout << "\n========== TASK 6: Download Manager ==========\n";

    DownloadManager manager(2, 4);
    vector<thread> workers;

    for (int i = 0; i < 3; i++) {
        workers.emplace_back([&manager]() { manager.download_worker(); });
    }

    manager.add_file(FileDownload(1, 5));
    manager.add_file(FileDownload(2, 4));
    manager.add_file(FileDownload(3, 3));

    for (auto& t : workers) {
        t.join();
    }

    cout << "Completed files: " << manager.get_completed_files() << "/3" << endl;
}

// MAIN MENU

int main() {
    cout << "========================================\n";
    cout << "SEMINAR 5: Semaphores\n";
    cout << "========================================\n";

    int choice;
    do {
        cout << "\n----------------------------------------\n";
        cout << "1 - Task 1 (Resource Pool with Priority)\n";
        cout << "2 - Task 2 (Parking Lot with VIP)\n";
        cout << "3 - Task 3 (Multiple Buffers)\n";
        cout << "4 - Task 4 (Printer Queue)\n";
        cout << "5 - Task 5 (Task Scheduler)\n";
        cout << "6 - Task 6 (Download Manager)\n";
        cout << "0 - Exit\n";
        cout << "----------------------------------------\n";
        cout << "Your choice: ";
        cin >> choice;

        switch (choice) {
        case 1: task1(); break;
        case 2: task2(); break;
        case 3: task3(); break;
        case 4: task4(); break;
        case 5: task5(); break;
        case 6: task6(); break;
        case 0: cout << "Exiting...\n"; break;
        default: cout << "Invalid choice!\n";
        }
    } while (choice != 0);

    return 0;
}