#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <random>
#include <map>
#include <queue>
#include <fstream>
#include <functional>
#include <iomanip>
#include <string>

using namespace std;
using namespace std::chrono;

// ЗАДАЧА 1: Параллельное суммирование массива

template <typename T>
class ParallelSum {
private:
    vector<T> data;
    size_t num_threads;
    T total_sum;
    mutex mtx;
    condition_variable cv;
    size_t completed_threads;

public:
    ParallelSum(const vector<T>& _data, size_t _num_threads)
        : data(_data), num_threads(_num_threads), total_sum(0), completed_threads(0) {
    }

    T compute_sum() {
        completed_threads = 0;
        total_sum = T(0);

        size_t data_size = data.size();
        size_t chunk_size = data_size / num_threads;

        for (size_t i = 0; i < num_threads; i++) {
            size_t start = i * chunk_size;
            size_t end = (i == num_threads - 1) ? data_size : start + chunk_size;

            thread([this, start, end]() {
                auto sum_segment = [](const vector<T>& vec, size_t s, size_t e) -> T {
                    T local_sum = T(0);
                    for (size_t j = s; j < e; j++) {
                        local_sum += vec[j];
                        if (j % 1000 == 0) this_thread::yield();
                    }
                    return local_sum;
                    };

                T local_sum = sum_segment(data, start, end);

                cout << "Thread " << this_thread::get_id() << " segment [" << start << "-" << end
                    << "] sum = " << local_sum << endl;

                {
                    lock_guard<mutex> lock(mtx);
                    total_sum += local_sum;
                    completed_threads++;
                }
                cv.notify_one();

                }).detach();
        }

        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this]() { return completed_threads == num_threads; });

        return total_sum;
    }
};

void task1() {
    cout << "\n========== TASK 1: Parallel Sum ==========\n";

    vector<int> data(1000000);
    for (size_t i = 0; i < data.size(); i++) data[i] = i + 1;

    ParallelSum<int> parallel_sum(data, 4);
    int result = parallel_sum.compute_sum();

    cout << "\nFINAL SUM: " << result << endl;
}

// ЗАДАЧА 2: Многопоточный банк с переводами

template <typename T>
class Account {
private:
    T balance;
    mutex mtx;
public:
    Account(T initial_balance = 0) : balance(initial_balance) {}

    T get_balance() {
        lock_guard<mutex> lock(mtx);
        return balance;
    }

    void deposit(T amount) {
        lock_guard<mutex> lock(mtx);
        balance += amount;
    }

    bool withdraw(T amount) {
        lock_guard<mutex> lock(mtx);
        if (balance >= amount) {
            balance -= amount;
            return true;
        }
        return false;
    }

    mutex& get_mutex() { return mtx; }
};

template <typename T>
class Bank {
private:
    vector<Account<T>> accounts;
    mutex global_mtx;
    condition_variable cv;

public:
    Bank(int num_accounts, T initial_balance) {
        for (int i = 0; i < num_accounts; i++) {
            accounts.push_back(Account<T>(initial_balance));
        }
    }

    T get_total_balance() {
        T total = 0;
        for (auto& acc : accounts) {
            total += acc.get_balance();
        }
        return total;
    }

    void transfer(int from, int to, T amount) {
        if (from == to) return;

        auto do_transfer = [this, from, to, amount]() -> bool {
            Account<T>& acc_from = accounts[from];
            Account<T>& acc_to = accounts[to];

            if (from < to) {
                lock(acc_from.get_mutex(), acc_to.get_mutex());
                lock_guard<mutex> lock1(acc_from.get_mutex(), adopt_lock);
                lock_guard<mutex> lock2(acc_to.get_mutex(), adopt_lock);
            }
            else {
                lock(acc_to.get_mutex(), acc_from.get_mutex());
                lock_guard<mutex> lock1(acc_to.get_mutex(), adopt_lock);
                lock_guard<mutex> lock2(acc_from.get_mutex(), adopt_lock);
            }

            if (acc_from.withdraw(amount)) {
                acc_to.deposit(amount);
                return true;
            }
            return false;
            };

        bool success = do_transfer();
        if (success) {
            cout << "Thread " << this_thread::get_id() << " transferred " << amount
                << " from " << from << " to " << to << endl;
        }
    }
};

void task2() {
    cout << "\n========== TASK 2: Multithreaded Bank ==========\n";

    Bank<int> bank(5, 1000);
    cout << "Initial total balance: " << bank.get_total_balance() << endl;

    vector<thread> threads;

    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&bank]() {
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<> accDist(0, 4);
            uniform_int_distribution<> amountDist(10, 200);

            for (int j = 0; j < 5; j++) {
                int from = accDist(gen);
                int to = accDist(gen);
                int amount = amountDist(gen);
                bank.transfer(from, to, amount);
                this_thread::sleep_for(milliseconds(10));
            }
            });
    }

    for (auto& t : threads) {
        t.join();
    }

    cout << "Final total balance: " << bank.get_total_balance() << endl;
}

// ЗАДАЧА 3: Производитель-Потребитель с буфером

template <typename T>
class Buffer {
private:
    vector<T> buffer;
    size_t capacity;
    mutex mtx;
    condition_variable cv_producer;
    condition_variable cv_consumer;

public:
    Buffer(size_t cap) : capacity(cap) {
        buffer.reserve(capacity);
    }

    void produce(T value) {
        unique_lock<mutex> lock(mtx);
        cv_producer.wait(lock, [this]() { return buffer.size() < capacity; });

        buffer.push_back(value);
        cout << "Producer " << this_thread::get_id() << " added: " << value
            << " (size: " << buffer.size() << ")" << endl;

        this_thread::yield();
        cv_consumer.notify_one();
    }

    T consume() {
        unique_lock<mutex> lock(mtx);
        cv_consumer.wait(lock, [this]() { return !buffer.empty(); });

        T value = buffer.front();
        buffer.erase(buffer.begin());
        cout << "Consumer " << this_thread::get_id() << " removed: " << value
            << " (size: " << buffer.size() << ")" << endl;

        this_thread::yield();
        cv_producer.notify_one();
        return value;
    }
};

void task3() {
    cout << "\n========== TASK 3: Producer-Consumer ==========\n";

    Buffer<int> buffer(5);

    vector<thread> producers;
    vector<thread> consumers;

    for (int i = 0; i < 3; i++) {
        producers.emplace_back([&buffer]() {
            for (int j = 0; j < 5; j++) {
                buffer.produce(j);
                this_thread::sleep_for(milliseconds(50));
            }
            });
    }

    for (int i = 0; i < 3; i++) {
        consumers.emplace_back([&buffer]() {
            for (int j = 0; j < 5; j++) {
                buffer.consume();
                this_thread::sleep_for(milliseconds(100));
            }
            });
    }

    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();
}

// ЗАДАЧА 4: Потоковое логирование (ИСПРАВЛЕНА)

template <typename T>
class Logger {
private:
    ofstream log_file;
    mutex mtx;

public:
    Logger(const string& filename) {
        log_file.open(filename, ios::out | ios::app);
        if (!log_file.is_open()) {
            cout << "Warning: Could not open log file" << endl;
        }
    }

    ~Logger() {
        if (log_file.is_open()) {
            log_file.close();
        }
    }

    void log(const T& message) {
        auto write_log = [this](const T& msg) {
            lock_guard<mutex> lock(mtx);
            if (log_file.is_open()) {
                log_file << msg << endl;
            }
            cout << "Thread " << this_thread::get_id() << " logged: " << msg << endl;
            };
        write_log(message);
    }
};

void task4() {
    cout << "\n========== TASK 4: Thread-safe Logger ==========\n";

    Logger<string> logger("log.txt");

    vector<thread> threads;

    for (int i = 0; i < 5; i++) {
        threads.emplace_back([&logger, i]() {
            for (int j = 0; j < 3; j++) {
                logger.log("Message from thread " + to_string(i) + ": " + to_string(j));
                this_thread::sleep_for(milliseconds(20));
            }
            });
    }

    for (auto& t : threads) {
        t.join();
    }

    cout << "Logging completed. Check log.txt" << endl;
}

// ЗАДАЧА 5: Потокобезопасный глобальный кэш

template <typename Key, typename Value>
class Cache {
private:
    map<Key, Value> data;
    mutex mtx;
    condition_variable cv;

public:
    void set(const Key& key, const Value& value) {
        lock_guard<mutex> lock(mtx);
        data[key] = value;
        cout << "Thread " << this_thread::get_id() << " set: " << key << " -> " << value << endl;
        cv.notify_all();
    }

    Value get(const Key& key) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this, &key]() {
            return data.find(key) != data.end();
            });
        return data[key];
    }

    void print_all() {
        lock_guard<mutex> lock(mtx);
        cout << "Cache contents:\n";
        for (const auto& [key, value] : data) {
            cout << "  " << key << " -> " << value << endl;
        }
    }
};

void task5() {
    cout << "\n========== TASK 5: Thread-safe Cache ==========\n";

    Cache<int, string> cache;

    vector<thread> writers;
    vector<thread> readers;

    writers.emplace_back([&cache]() {
        this_thread::sleep_for(milliseconds(100));
        cache.set(1, "Hello");
        });

    writers.emplace_back([&cache]() {
        this_thread::sleep_for(milliseconds(200));
        cache.set(2, "World");
        });

    readers.emplace_back([&cache]() {
        cout << "Reader got: " << cache.get(1) << endl;
        });

    readers.emplace_back([&cache]() {
        cout << "Reader got: " << cache.get(2) << endl;
        });

    for (auto& t : writers) t.join();
    for (auto& t : readers) t.join();

    cache.print_all();
}

// ЗАДАЧА 6: Многопоточная очередь с приоритетом

template <typename T>
struct PriorityItem {
    T value;
    int priority;
    bool operator<(const PriorityItem& other) const {
        return priority < other.priority;
    }
};

template <typename T>
class PriorityQueue {
private:
    priority_queue<PriorityItem<T>> queue;
    mutex mtx;
    condition_variable cv;

public:
    void push(T value, int priority) {
        lock_guard<mutex> lock(mtx);
        queue.push({ value, priority });
        cout << "Thread " << this_thread::get_id() << " pushed: " << value
            << " (priority " << priority << ")" << endl;
        cv.notify_one();
    }

    T pop() {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this]() { return !queue.empty(); });
        T value = queue.top().value;
        queue.pop();
        cout << "Thread " << this_thread::get_id() << " popped: " << value << endl;
        this_thread::yield();
        return value;
    }
};

void task6() {
    cout << "\n========== TASK 6: Priority Queue ==========\n";

    PriorityQueue<string> pq;

    vector<thread> producers;
    vector<thread> consumers;

    producers.emplace_back([&pq]() { pq.push("Low", 1); });
    producers.emplace_back([&pq]() { pq.push("Medium", 5); });
    producers.emplace_back([&pq]() { pq.push("High", 10); });
    producers.emplace_back([&pq]() { pq.push("Urgent", 20); });

    consumers.emplace_back([&pq]() { pq.pop(); });
    consumers.emplace_back([&pq]() { pq.pop(); });
    consumers.emplace_back([&pq]() { pq.pop(); });
    consumers.emplace_back([&pq]() { pq.pop(); });

    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();
}



int main() {
    cout << "========================================\n";
    cout << "SEMINAR 4: Multithreading\n";
    cout << "========================================\n";

    int choice;
    do {
        cout << "\n----------------------------------------\n";
        cout << "1 - Task 1 (Parallel Sum)\n";
        cout << "2 - Task 2 (Multithreaded Bank)\n";
        cout << "3 - Task 3 (Producer-Consumer Buffer)\n";
        cout << "4 - Task 4 (Thread-safe Logger)\n";
        cout << "5 - Task 5 (Thread-safe Cache)\n";
        cout << "6 - Task 6 (Priority Queue)\n";
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
