#include <iostream>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <functional>
#include <random>

using namespace std;
using namespace std::chrono;

enum TaskType {
    FACTORIAL,
    FIBONACCI,
    DIGIT_SUM,
    IS_PRIME,
    GCD,
    REVERSE_NUMBER
};

struct Task {
    int id;
    TaskType type;
    int arg1;
    int arg2;  
    int priority;

    Task(int i, TaskType t, int a1, int a2 = 0, int prio = 1)
        : id(i), type(t), arg1(a1), arg2(a2), priority(prio) {
    }

    bool operator<(const Task& other) const {
        return priority < other.priority;  
    }
};

struct Result {
    int task_id;
    TaskType type;
    int input1;
    int input2;
    long long output;
    double execution_time;
    bool success;
    string error_message;

    Result() : task_id(-1), input1(0), input2(0), output(0), execution_time(0), success(true) {}
};


class TaskProcessor {
private:
    priority_queue<Task> task_queue;  
    vector<Result> results;

    mutex queue_mutex;
    mutex results_mutex;
    condition_variable cv;

    atomic<int> active_threads;
    atomic<int> completed_tasks;
    atomic<bool> finished;
    atomic<bool> stop_workers;

    int num_threads;
    int total_tasks;
    steady_clock::time_point start_time;

    random_device rd;
    mt19937 gen;
    uniform_int_distribution<> task_dist;

public:
    TaskProcessor(int threads = 3)
        : num_threads(threads),
        active_threads(0),
        completed_tasks(0),
        finished(false),
        stop_workers(false),
        total_tasks(0),
        gen(rd()),
        task_dist(0, 5) {
    }


    long long factorial(int n) {
        if (n < 0) throw runtime_error("Negative number");
        if (n > 20) throw runtime_error("Number too large (max 20)");
        long long result = 1;
        for (int i = 2; i <= n; i++) {
            result *= i;
        }
        return result;
    }

    long long fibonacci(int n) {
        if (n < 0) throw runtime_error("Negative number");
        if (n == 0) return 0;
        if (n == 1) return 1;
        long long a = 0, b = 1;
        for (int i = 2; i <= n; i++) {
            long long c = a + b;
            a = b;
            b = c;
        }
        return b;
    }

    long long digitSum(int n) {
        n = abs(n);
        long long sum = 0;
        while (n > 0) {
            sum += n % 10;
            n /= 10;
        }
        return sum;
    }

    long long isPrime(int n) {
        if (n <= 1) return 0;
        if (n <= 3) return 1;
        if (n % 2 == 0 || n % 3 == 0) return 0;
        for (int i = 5; i * i <= n; i += 6) {
            if (n % i == 0 || n % (i + 2) == 0) return 0;
        }
        return 1;
    }

    long long gcd(int a, int b) {
        a = abs(a);
        b = abs(b);
        while (b != 0) {
            int temp = b;
            b = a % b;
            a = temp;
        }
        return a;
    }

    long long reverseNumber(int n) {
        int sign = (n < 0) ? -1 : 1;
        n = abs(n);
        long long reversed = 0;
        while (n > 0) {
            reversed = reversed * 10 + (n % 10);
            n /= 10;
        }
        return reversed * sign;
    }

    Result execute_task(const Task& task) {
        Result res;
        res.task_id = task.id;
        res.type = task.type;
        res.input1 = task.arg1;
        res.input2 = task.arg2;

        auto start = steady_clock::now();

        try {
            switch (task.type) {
            case FACTORIAL:
                res.output = factorial(task.arg1);
                break;
            case FIBONACCI:
                res.output = fibonacci(task.arg1);
                break;
            case DIGIT_SUM:
                res.output = digitSum(task.arg1);
                break;
            case IS_PRIME:
                res.output = isPrime(task.arg1);
                break;
            case GCD:
                res.output = gcd(task.arg1, task.arg2);
                break;
            case REVERSE_NUMBER:
                res.output = reverseNumber(task.arg1);
                break;
            }
            res.success = true;
        }
        catch (const exception& e) {
            res.success = false;
            res.error_message = e.what();
        }

        auto end = steady_clock::now();
        res.execution_time = duration_cast<milliseconds>(end - start).count() / 1000.0;

        return res;
    }

    void add_task(const Task& task) {
        lock_guard<mutex> lock(queue_mutex);
        task_queue.push(task);
        total_tasks++;
        cout << "[MAIN] Added task " << task.id << " (type: " << get_task_name(task.type)
            << ", priority: " << task.priority << ")" << endl;
        cv.notify_one();
    }

    void add_random_tasks(int count) {
        vector<pair<TaskType, string>> task_types = {
            {FACTORIAL, "Factorial"},
            {FIBONACCI, "Fibonacci"},
            {DIGIT_SUM, "Digit Sum"},
            {IS_PRIME, "Is Prime"},
            {GCD, "GCD"},
            {REVERSE_NUMBER, "Reverse Number"}
        };

        uniform_int_distribution<> num_dist(1, 30);
        uniform_int_distribution<> priority_dist(1, 5);

        for (int i = 0; i < count; i++) {
            int type_idx = task_dist(gen);
            TaskType type = task_types[type_idx].first;
            int arg1 = num_dist(gen);
            int arg2 = (type == GCD) ? num_dist(gen) : 0;
            int priority = priority_dist(gen);

            add_task(Task(i + 1, type, arg1, arg2, priority));
        }
    }

    string get_task_name(TaskType type) {
        switch (type) {
        case FACTORIAL: return "Factorial";
        case FIBONACCI: return "Fibonacci";
        case DIGIT_SUM: return "Digit Sum";
        case IS_PRIME: return "Is Prime";
        case GCD: return "GCD";
        case REVERSE_NUMBER: return "Reverse Number";
        default: return "Unknown";
        }
    }

    void worker_thread(int thread_id) {
        active_threads++;

        while (!stop_workers) {
            Task task(0, FACTORIAL, 0);
            bool has_task = false;

            {
                unique_lock<mutex> lock(queue_mutex);
                cv.wait(lock, [this]() {
                    return !task_queue.empty() || stop_workers;
                    });

                if (stop_workers && task_queue.empty()) {
                    break;
                }

                if (!task_queue.empty()) {
                    task = task_queue.top();
                    task_queue.pop();
                    has_task = true;
                }
            }

            if (has_task) {
                cout << "[Thread " << thread_id << "] Executing task " << task.id
                    << ": " << get_task_name(task.type) << "(" << task.arg1;
                if (task.type == GCD) cout << ", " << task.arg2;
                cout << ")" << endl;

                Result res = execute_task(task);

                {
                    lock_guard<mutex> lock(results_mutex);
                    results.push_back(res);
                    completed_tasks++;
                }

                cout << "[Thread " << thread_id << "] Task " << task.id << " completed"
                    << " -> Result: " << res.output
                    << " (time: " << fixed << setprecision(3) << res.execution_time << " sec)" << endl;

                this_thread::yield();
            }
        }

        active_threads--;
    }

    void run() {
        start_time = steady_clock::now();

        vector<thread> threads;

        cout << "\n========================================\n";
        cout << "Starting task processing with " << num_threads << " threads...\n";
        cout << "========================================\n\n";

        for (int i = 0; i < num_threads; i++) {
            threads.emplace_back(&TaskProcessor::worker_thread, this, i + 1);
        }

        {
            unique_lock<mutex> lock(queue_mutex);
            cv.wait(lock, [this]() {
                return task_queue.empty() && completed_tasks >= total_tasks;
                });
        }

        stop_workers = true;
        cv.notify_all();

        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }

        auto end_time = steady_clock::now();
        double total_time = duration_cast<milliseconds>(end_time - start_time).count() / 1000.0;

        print_results(total_time);
    }

    void print_results(double total_time) {
        cout << "\n========================================\n";
        cout << "TASK PROCESSING COMPLETED\n";
        cout << "========================================\n";
        cout << "Total tasks: " << completed_tasks << "\n";
        cout << "Total time: " << fixed << setprecision(3) << total_time << " seconds\n";
        cout << "\n========================================\n";
        cout << "RESULTS:\n";
        cout << "========================================\n\n";

        for (const auto& res : results) {
            cout << "Task " << res.task_id << ": " << get_task_name(res.type) << "(" << res.input1;
            if (res.type == GCD) cout << ", " << res.input2;
            cout << ")";

            if (res.success) {
                if (res.type == IS_PRIME) {
                    cout << " -> " << (res.output ? "PRIME" : "NOT PRIME");
                }
                else {
                    cout << " -> " << res.output;
                }
            }
            else {
                cout << " -> ERROR: " << res.error_message;
            }
            cout << " [" << res.execution_time << " sec]" << endl;
        }

        cout << "\n========================================\n";
    }

    int get_completed_count() const { return completed_tasks.load(); }
};


int main() {
    cout << "========================================\n";
    cout << "HOMEWORK 1 - VARIANT 3\n";
    cout << "Task Queue with Priority\n";
    cout << "========================================\n";

    TaskProcessor processor(3);

    processor.add_random_tasks(10);

    processor.run();

    cout << "\nProgram finished successfully!\n";

    return 0;
}