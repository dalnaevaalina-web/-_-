#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>
#include <random>
#include <iomanip>
#include <queue>

using namespace std;
using namespace std::chrono;

// ЗАДАЧА 1

long long sumFrom1ToN(long long N) {
    long long sum = 0;
    for (long long i = 1; i <= N; i++) {
        sum += i;
    }
    return sum;
}

void task1() {
    cout << "\n========== TASK 1 ==========\n";
    long long N;
    cout << "Enter N: ";
    cin >> N;

    auto start = high_resolution_clock::now();
    long long result = sumFrom1ToN(N);
    auto end = high_resolution_clock::now();

    auto duration = duration_cast<milliseconds>(end - start);
    cout << "Sum from 1 to " << N << " = " << result << endl;
    cout << "Time: " << duration.count() << " ms" << endl;
}

// ЗАДАЧА 2

void task2() {
    cout << "\n========== TASK 2 ==========\n";
    int N;
    cout << "Enter seconds: ";
    cin >> N;

    for (int i = N; i >= 1; i--) {
        cout << "Remaining: " << i << " seconds" << endl;
        this_thread::sleep_for(seconds(1));
    }
    cout << "Time's up!" << endl;
}

// ЗАДАЧА 3

void task3() {
    cout << "\n========== TASK 3 ==========\n";
    int totalSeconds;
    cout << "Enter seconds: ";
    cin >> totalSeconds;

    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    cout << hours << " hours " << minutes << " minutes " << seconds << " seconds" << endl;
}

// ЗАДАЧА 4

class TaskTimer {
private:
    time_point<high_resolution_clock> startTime;
    time_point<high_resolution_clock> endTime;
    bool running;

public:
    TaskTimer() : running(false) {}

    void start() {
        startTime = high_resolution_clock::now();
        running = true;
    }

    void stop() {
        endTime = high_resolution_clock::now();
        running = false;
    }

    long long getDurationMs() {
        if (running) return 0;
        return duration_cast<milliseconds>(endTime - startTime).count();
    }

    void measureSorting(vector<int>& arr) {
        start();
        sort(arr.begin(), arr.end());
        stop();
        cout << "Sorting time: " << getDurationMs() << " ms" << endl;
    }
};

void task4() {
    cout << "\n========== TASK 4 ==========\n";
    const int SIZE = 100000;
    vector<int> arr(SIZE);

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(1, 1000);

    for (int i = 0; i < SIZE; i++) {
        arr[i] = dist(gen);
    }

    TaskTimer timer;
    timer.measureSorting(arr);
    timer.measureSorting(arr);
}

// ЗАДАЧА 5

void bubbleSort(vector<int>& arr) {
    int n = arr.size();
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                swap(arr[j], arr[j + 1]);
            }
        }
    }
}

void insertionSort(vector<int>& arr) {
    int n = arr.size();
    for (int i = 1; i < n; i++) {
        int key = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

void mergeArrays(vector<int>& arr, int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    vector<int> L(n1), R(n2);
    for (int i = 0; i < n1; i++) L[i] = arr[left + i];
    for (int i = 0; i < n2; i++) R[i] = arr[mid + 1 + i];
    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) arr[k++] = L[i++];
        else arr[k++] = R[j++];
    }
    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];
}

void mergeSort(vector<int>& arr, int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        mergeSort(arr, left, mid);
        mergeSort(arr, mid + 1, right);
        mergeArrays(arr, left, mid, right);
    }
}

int partitionArray(vector<int>& arr, int low, int high) {
    int pivot = arr[high];
    int i = low - 1;
    for (int j = low; j < high; j++) {
        if (arr[j] <= pivot) {
            i++;
            swap(arr[i], arr[j]);
        }
    }
    swap(arr[i + 1], arr[high]);
    return i + 1;
}

void quickSort(vector<int>& arr, int low, int high) {
    if (low < high) {
        int pi = partitionArray(arr, low, high);
        quickSort(arr, low, pi - 1);
        quickSort(arr, pi + 1, high);
    }
}

void task5() {
    cout << "\n========== TASK 5 ==========\n";
    const int SIZE = 10000;
    vector<int> original(SIZE);

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(1, 10000);

    for (int i = 0; i < SIZE; i++) {
        original[i] = dist(gen);
    }

    // Bubble Sort
    vector<int> arr1 = original;
    auto start = high_resolution_clock::now();
    bubbleSort(arr1);
    auto end = high_resolution_clock::now();
    cout << "Bubble Sort: " << duration_cast<milliseconds>(end - start).count() << " ms" << endl;

    // Insertion Sort
    vector<int> arr2 = original;
    start = high_resolution_clock::now();
    insertionSort(arr2);
    end = high_resolution_clock::now();
    cout << "Insertion Sort: " << duration_cast<milliseconds>(end - start).count() << " ms" << endl;

    // Merge Sort
    vector<int> arr3 = original;
    start = high_resolution_clock::now();
    mergeSort(arr3, 0, arr3.size() - 1);
    end = high_resolution_clock::now();
    cout << "Merge Sort: " << duration_cast<milliseconds>(end - start).count() << " ms" << endl;

    // Quick Sort
    vector<int> arr4 = original;
    start = high_resolution_clock::now();
    quickSort(arr4, 0, arr4.size() - 1);
    end = high_resolution_clock::now();
    cout << "Quick Sort: " << duration_cast<milliseconds>(end - start).count() << " ms" << endl;

    // std::sort
    vector<int> arr5 = original;
    start = high_resolution_clock::now();
    sort(arr5.begin(), arr5.end());
    end = high_resolution_clock::now();
    cout << "std::sort: " << duration_cast<milliseconds>(end - start).count() << " ms" << endl;
}

// ЗАДАЧА 6

class VirtualThreadV6 {
private:
    vector<int> numbers;
    int index;
    int threadNum;
public:
    VirtualThreadV6(const vector<int>& nums, int num) : numbers(nums), index(0), threadNum(num) {}

    bool hasTasks() {
        return index < numbers.size();
    }

    void run() {
        if (hasTasks()) {
            int num = numbers[index];
            long long fact = 1;
            for (int i = 1; i <= num; i++) {
                fact *= i;
            }
            cout << "Virtual thread " << threadNum << " computes " << num << "! = " << fact << endl;
            index++;
        }
    }
};

void task6() {
    cout << "\n========== TASK 6 ==========\n";
    VirtualThreadV6 vt1({ 5, 10 }, 1);
    VirtualThreadV6 vt2({ 7, 12 }, 2);

    while (vt1.hasTasks() || vt2.hasTasks()) {
        if (vt1.hasTasks()) vt1.run();
        if (vt2.hasTasks()) vt2.run();
    }
}

// ЗАДАЧА 7

class VirtualThreadV7 {
private:
    queue<string> tasks;
    int threadId;
public:
    VirtualThreadV7(const vector<string>& t, int id) : threadId(id) {
        for (const auto& task : t) tasks.push(task);
    }

    bool hasTasks() { return !tasks.empty(); }

    void runNextTask() {
        if (hasTasks()) {
            string task = tasks.front();
            tasks.pop();
            cout << "Virtual thread " << threadId << " started task: " << task << endl;
            this_thread::sleep_for(milliseconds(500));
            cout << "Virtual thread " << threadId << " finished task: " << task << endl;
        }
    }
};

void task7() {
    cout << "\n========== TASK 7 ==========\n";
    VirtualThreadV7 vt1({ "Task A", "Task C" }, 1);
    VirtualThreadV7 vt2({ "Task B", "Task D" }, 2);

    while (vt1.hasTasks() || vt2.hasTasks()) {
        if (vt1.hasTasks()) vt1.runNextTask();
        if (vt2.hasTasks()) vt2.runNextTask();
    }
}

// ЗАДАЧА 8

struct TaskV8 {
    int value;
    int priority;
    int duration_ms;
    int steps;
    int currentStep;
    int result;

    TaskV8(int v, int p, int d, int s)
        : value(v), priority(p), duration_ms(d), steps(s), currentStep(0), result(0) {
    }

    bool isComplete() const { return currentStep >= steps; }

    void executeStep() {
        if (isComplete()) return;
        currentStep++;
        int stepTime = duration_ms / steps;
        this_thread::sleep_for(milliseconds(stepTime));
        if (currentStep == steps) {
            result = value * value;
        }
    }
};

void task8() {
    cout << "\n========== TASK 8 ==========\n";

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> valueDist(1, 50);
    uniform_int_distribution<> priorityDist(1, 10);
    uniform_int_distribution<> durationDist(200, 1000);
    uniform_int_distribution<> stepsDist(2, 5);

    vector<TaskV8> tasks1, tasks2;

    for (int i = 0; i < 3; i++) {
        tasks1.push_back(TaskV8(valueDist(gen), priorityDist(gen), durationDist(gen), stepsDist(gen)));
        tasks2.push_back(TaskV8(valueDist(gen), priorityDist(gen), durationDist(gen), stepsDist(gen)));
    }

    bool tasks1Done = false, tasks2Done = false;

    while (!tasks1Done || !tasks2Done) {
        if (!tasks1Done) {
            int bestIndex = -1;
            int bestPriority = -1;
            for (int i = 0; i < tasks1.size(); i++) {
                if (!tasks1[i].isComplete() && tasks1[i].priority > bestPriority) {
                    bestPriority = tasks1[i].priority;
                    bestIndex = i;
                }
            }
            if (bestIndex != -1) {
                tasks1[bestIndex].executeStep();
                cout << "Virtual thread 1 executes step " << tasks1[bestIndex].currentStep
                    << "/" << tasks1[bestIndex].steps << " of task " << tasks1[bestIndex].value
                    << " (priority " << tasks1[bestIndex].priority << ")" << endl;
                if (tasks1[bestIndex].isComplete()) {
                    cout << "Virtual thread 1 completed task " << tasks1[bestIndex].value
                        << ": result = " << tasks1[bestIndex].result << endl;
                }
            }
            else {
                tasks1Done = true;
            }
        }

        if (!tasks2Done) {
            int bestIndex = -1;
            int bestPriority = -1;
            for (int i = 0; i < tasks2.size(); i++) {
                if (!tasks2[i].isComplete() && tasks2[i].priority > bestPriority) {
                    bestPriority = tasks2[i].priority;
                    bestIndex = i;
                }
            }
            if (bestIndex != -1) {
                tasks2[bestIndex].executeStep();
                cout << "Virtual thread 2 executes step " << tasks2[bestIndex].currentStep
                    << "/" << tasks2[bestIndex].steps << " of task " << tasks2[bestIndex].value
                    << " (priority " << tasks2[bestIndex].priority << ")" << endl;
                if (tasks2[bestIndex].isComplete()) {
                    cout << "Virtual thread 2 completed task " << tasks2[bestIndex].value
                        << ": result = " << tasks2[bestIndex].result << endl;
                }
            }
            else {
                tasks2Done = true;
            }
        }
    }
}


int main() {
    cout << "========================================\n";
    cout << "SEMINAR 3: Chrono, Random, Hyper-Threading\n";
    cout << "========================================\n";

    int choice;
    do {
        cout << "\n----------------------------------------\n";
        cout << "1 - Task 1 (Measure function time)\n";
        cout << "2 - Task 2 (Countdown timer)\n";
        cout << "3 - Task 3 (Time converter)\n";
        cout << "4 - Task 4 (TaskTimer class)\n";
        cout << "5 - Task 5 (Sorting algorithms comparison)\n";
        cout << "6 - Task 6 (HT: Factorials)\n";
        cout << "7 - Task 7 (HT: Task queue)\n";
        cout << "8 - Task 8 (HT: Priority + steps)\n";
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
        case 7: task7(); break;
        case 8: task8(); break;
        case 0: cout << "Exiting...\n"; break;
        default: cout << "Invalid choice!\n";
        }
    } while (choice != 0);

    return 0;
}