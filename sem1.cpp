#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <string>
#include <fstream>
#include <cstring>

using namespace std;

// ЗАДАЧА 1


double getAverage(const double* arr, size_t size) {
    if (size == 0) return 0;
    double sum = 0;
    for (size_t i = 0; i < size; i++) sum += arr[i];
    return sum / size;
}

double getMax(const double* arr, size_t size) {
    if (size == 0) return 0;
    double maxVal = arr[0];
    for (size_t i = 1; i < size; i++) if (arr[i] > maxVal) maxVal = arr[i];
    return maxVal;
}

double getMin(const double* arr, size_t size) {
    if (size == 0) return 0;
    double minVal = arr[0];
    for (size_t i = 1; i < size; i++) if (arr[i] < minVal) minVal = arr[i];
    return minVal;
}

size_t countAboveThreshold(const double* arr, size_t size, double threshold) {
    size_t count = 0;
    for (size_t i = 0; i < size; i++) if (arr[i] > threshold) count++;
    return count;
}

void task1() {
    cout << "\n========== TASK 1 ==========\n";
    int N;
    do {
        cout << "Enter number of students (N > 0): ";
        cin >> N;
    } while (N <= 0);

    double* grades = new double[N];

    for (int i = 0; i < N; i++) {
        do {
            cout << "Student " << i + 1 << " grade (0-5): ";
            cin >> grades[i];
        } while (grades[i] < 0 || grades[i] > 5);
    }

    cout << "Average: " << getAverage(grades, N) << endl;
    cout << "Maximum: " << getMax(grades, N) << endl;
    cout << "Minimum: " << getMin(grades, N) << endl;

    double threshold;
    cout << "Enter threshold: ";
    cin >> threshold;
    cout << "Students above threshold: " << countAboveThreshold(grades, N, threshold) << endl;

    delete[] grades;
}


// ЗАДАЧА 2


void task2() {
    cout << "\n========== TASK 2 ==========\n";
    int N, M;
    cout << "Number of students: ";
    cin >> N;
    cout << "Number of subjects: ";
    cin >> M;

    vector<vector<double>> students(N, vector<double>(M));
    vector<double> studentAvg(N, 0);
    vector<double> subjectAvg(M, 0);

    for (int i = 0; i < N; i++) {
        cout << "\nStudent " << i + 1 << ":\n";
        double studentSum = 0;
        for (int j = 0; j < M; j++) {
            do {
                cout << "  Subject " << j + 1 << " grade (0-5): ";
                cin >> students[i][j];
            } while (students[i][j] < 0 || students[i][j] > 5);
            studentSum += students[i][j];
            subjectAvg[j] += students[i][j];
        }
        studentAvg[i] = studentSum / M;
    }

    for (int j = 0; j < M; j++) subjectAvg[j] /= N;

    cout << "\n--- Student averages ---\n";
    for (int i = 0; i < N; i++)
        cout << "Student " << i + 1 << ": " << fixed << setprecision(2) << studentAvg[i] << endl;

    cout << "\n--- Subject averages ---\n";
    for (int j = 0; j < M; j++)
        cout << "Subject " << j + 1 << ": " << subjectAvg[j] << endl;

    int maxIdx = 0;
    for (int i = 1; i < N; i++)
        if (studentAvg[i] > studentAvg[maxIdx]) maxIdx = i;
    cout << "\nStudent with highest average: #" << maxIdx + 1 << " (" << studentAvg[maxIdx] << ")\n";
}


// ЗАДАЧА 3


void task3() {
    cout << "\n========== TASK 3 ==========\n";
    int N;
    cout << "Number of students: ";
    cin >> N;

    vector<double> averages(N);
    for (int i = 0; i < N; i++) {
        cout << "Student " << i + 1 << " average: ";
        cin >> averages[i];
    }

    vector<pair<int, double>> students;
    for (int i = 0; i < N; i++)
        students.push_back({ i, averages[i] });

    sort(students.begin(), students.end(),
        [](const pair<int, double>& a, const pair<int, double>& b) {
            if (a.second != b.second) return a.second > b.second;
            return a.first < b.first;
        });

    cout << "\nSorted list (descending by average):\n";
    for (const auto& p : students)
        cout << "Student " << p.first + 1 << ": " << p.second << endl;

    // Demonstration of lambda captures
    double someValue = 4.0;
    auto lambdaByValue = [someValue](double x) { return x > someValue; };
    auto lambdaByRef = [&someValue](double x) { return x < someValue; };
    cout << "\nLambda capture demo: someValue = " << someValue << endl;
}


// ЗАДАЧА 4


void task4() {
    cout << "\n========== TASK 4 ==========\n";
    int N;
    cout << "Number of students: ";
    cin >> N;

    vector<pair<int, double>> students;
    for (int i = 0; i < N; i++) {
        double avg;
        cout << "Student " << i + 1 << " average: ";
        cin >> avg;
        students.push_back({ i, avg });
    }

    double threshold;
    cout << "Threshold for removal: ";
    cin >> threshold;

    int excellent = 0, bad = 0;
    for (const auto& s : students) {
        if (s.second >= 4.5) excellent++;
        else if (s.second < 3.0) bad++;
    }

    auto it = remove_if(students.begin(), students.end(),
        [threshold](const pair<int, double>& s) { return s.second < threshold; });
    students.erase(it, students.end());

    cout << "\nExcellent students (>=4.5): " << excellent << endl;
    cout << "At risk students (<3.0): " << bad << endl;
    cout << "Students left after filtering: " << students.size() << endl;
}

// ЗАДАЧА 5

#pragma pack(push, 1)
struct FileHeader {
    char signature[4];
    int version;
    int studentCount;
};
#pragma pack(pop)

void task5() {
    cout << "\n========== TASK 5 ==========\n";
    cout << "FileHeader structure size: " << sizeof(FileHeader) << " bytes\n";
    cout << "Signature 'GRPS', version 1\n";

    FileHeader header = { {'G','R','P','S'}, 1, 3 };
    ofstream file("test.bin", ios::binary);
    if (file) {
        file.write(reinterpret_cast<const char*>(&header), sizeof(header));
        file.close();
        cout << "File test.bin created\n";

        ifstream infile("test.bin", ios::binary);
        FileHeader readHeader;
        infile.read(reinterpret_cast<char*>(&readHeader), sizeof(readHeader));
        infile.close();

        if (memcmp(readHeader.signature, "GRPS", 4) == 0)
            cout << "Signature valid, file OK\n";
        else
            cout << "Signature error\n";
    }
}

// ЗАДАЧА 6

class StudentV6 {
private:
    string name;
    vector<double> grades;
public:
    explicit StudentV6(const string& n) : name(n) {}
    StudentV6(const string& n, const vector<double>& g) : name(n), grades(g) {}
    double getAverage() const {
        if (grades.empty()) return 0;
        double sum = 0;
        for (double g : grades) sum += g;
        return sum / grades.size();
    }
    void addGrade(double grade) {
        if (grade >= 0 && grade <= 5) grades.push_back(grade);
    }
    void print() const {
        cout << "Student: " << name << ", average: " << getAverage() << endl;
    }
    string getName() const { return name; }
};

void task6() {
    cout << "\n========== TASK 6 ==========\n";
    StudentV6 s1("Ivanov Ivan");
    s1.addGrade(5);
    s1.addGrade(4);
    s1.addGrade(5);
    s1.print();

    StudentV6 s2("Petrova Anna", { 5, 5, 4, 5 });
    s2.print();
}

// ЗАДАЧА 7

class PersonV7 {
protected:
    string name;
public:
    explicit PersonV7(const string& n) : name(n) {}
    virtual ~PersonV7() {}
    virtual void print() const { cout << "Name: " << name; }
    string getName() const { return name; }
};

class StudentV7 : public PersonV7 {
private:
    vector<double> grades;
public:
    StudentV7(const string& n) : PersonV7(n) {}
    void addGrade(double g) { if (g >= 0 && g <= 5) grades.push_back(g); }
    double getAverage() const {
        if (grades.empty()) return 0;
        double sum = 0;
        for (double g : grades) sum += g;
        return sum / grades.size();
    }
    void print() const override {
        PersonV7::print();
        cout << ", student, average: " << getAverage() << endl;
    }
};

class TeacherV7 : public PersonV7 {
private:
    string discipline;
public:
    TeacherV7(const string& n, const string& d) : PersonV7(n), discipline(d) {}
    void print() const override {
        PersonV7::print();
        cout << ", teacher, discipline: " << discipline << endl;
    }
};

void task7() {
    cout << "\n========== TASK 7 ==========\n";
    vector<PersonV7*> people;
    people.push_back(new StudentV7("Ivanov"));
    people.push_back(new TeacherV7("Petrova", "Mathematics"));

    dynamic_cast<StudentV7*>(people[0])->addGrade(5);
    dynamic_cast<StudentV7*>(people[0])->addGrade(4);

    for (auto p : people) {
        p->print();
        delete p;
    }
}

// ЗАДАЧА 8 (Композиция)

class RecordBookV8 {
private:
    int number;
    vector<double> grades;
public:
    RecordBookV8(int num) : number(num) {}
    void addGrade(double g) { if (g >= 0 && g <= 5) grades.push_back(g); }
    double getAverage() const {
        if (grades.empty()) return 0;
        double sum = 0;
        for (double g : grades) sum += g;
        return sum / grades.size();
    }
    void print() const {
        cout << "Record book #" << number << ", grades: ";
        for (double g : grades) cout << g << " ";
    }
    int getNumber() const { return number; }
};

class StudentV8 : public PersonV7 {
private:
    RecordBookV8 recordBook;
public:
    StudentV8(const string& name, int recordNum) : PersonV7(name), recordBook(recordNum) {}
    void addGrade(double g) { recordBook.addGrade(g); }
    double getAverage() const { return recordBook.getAverage(); }
    void print() const override {
        PersonV7::print();
        cout << ", ";
        recordBook.print();
        cout << ", average: " << getAverage() << endl;
    }
};

void task8() {
    cout << "\n========== TASK 8 (Composition) ==========\n";
    StudentV8 s("Sidorov Petr", 12345);
    s.addGrade(5);
    s.addGrade(4);
    s.addGrade(5);
    s.print();
}

// ЗАДАЧА 9 (Агрегация)

class GroupV9 {
private:
    string name;
    vector<StudentV8*> students;
public:
    GroupV9(const string& n) : name(n) {}
    void addStudent(StudentV8* s) { students.push_back(s); }
    bool removeStudent(const string& sname) {
        auto it = remove_if(students.begin(), students.end(),
            [&sname](StudentV8* s) { return s->getName() == sname; });
        if (it == students.end()) return false;
        students.erase(it, students.end());
        return true;
    }
    double getGroupAverage() const {
        if (students.empty()) return 0;
        double sum = 0;
        for (auto s : students) sum += s->getAverage();
        return sum / students.size();
    }
    void printAll() const {
        cout << "\n=== Group " << name << " ===\n";
        for (auto s : students) s->print();
    }
    void sortByAverage() {
        sort(students.begin(), students.end(),
            [](StudentV8* a, StudentV8* b) { return a->getAverage() > b->getAverage(); });
    }
};

void task9() {
    cout << "\n========== TASK 9 (Aggregation) ==========\n";
    StudentV8 s1("Alexeev", 1001);
    StudentV8 s2("Borisova", 1002);
    StudentV8 s3("Vasiliev", 1003);

    s1.addGrade(5); s1.addGrade(5);
    s2.addGrade(4); s2.addGrade(4);
    s3.addGrade(3); s3.addGrade(3);

    GroupV9 group("IU7-11B");
    group.addStudent(&s1);
    group.addStudent(&s2);
    group.addStudent(&s3);

    group.printAll();
    cout << "\nGroup average: " << group.getGroupAverage() << endl;

    group.sortByAverage();
    cout << "\nAfter sorting by average:\n";
    group.printAll();
}

// ЗАДАЧА 10 (inline метод)

class MathUtils {
public:
    static inline int square(int x) { return x * x; }
};

void task10() {
    cout << "\n========== TASK 10 (inline method) ==========\n";
    cout << "Square of 5: " << MathUtils::square(5) << endl;
    cout << "(In real project, classes are split into .hpp and .cpp files)\n";
}

// ЗАДАЧА 11 (Full program with menu and all features)

void task11() {
    cout << "\n========== TASK 11 ==========\n";

    StudentV8* s1 = new StudentV8("Anna Ivanova", 1001);
    StudentV8* s2 = new StudentV8("Boris Petrov", 1002);
    StudentV8* s3 = new StudentV8("Viktor Sidorov", 1003);
    StudentV8* s4 = new StudentV8("Galina Smirnova", 1004);

    s1->addGrade(5); s1->addGrade(5); s1->addGrade(4);
    s2->addGrade(3); s2->addGrade(4); s2->addGrade(3);
    s3->addGrade(2); s3->addGrade(3); s3->addGrade(2);
    s4->addGrade(5); s4->addGrade(5); s4->addGrade(5);

    GroupV9 group("IU7-11B");
    group.addStudent(s1);
    group.addStudent(s2);
    group.addStudent(s3);
    group.addStudent(s4);

    group.printAll();
    cout << "\nGroup average: " << group.getGroupAverage() << endl;

    group.sortByAverage();
    cout << "\nAfter sorting by average:\n";
    group.printAll();

    int excellent = 0, bad = 0;
    vector<StudentV8*> studentsVec = { s1, s2, s3, s4 };
    for (auto s : studentsVec) {
        if (s->getAverage() >= 4.5) excellent++;
        else if (s->getAverage() < 3.0) bad++;
    }
    cout << "\nStatistics:\n";
    cout << "Excellent students: " << excellent << endl;
    cout << "At risk students: " << bad << endl;

    cout << "\nSaving to file group.bin...\n";
    ofstream file("group.bin", ios::binary);
    if (file) {
        int version = 1;
        int count = 4;
        file.write(reinterpret_cast<const char*>(&version), sizeof(version));
        file.write(reinterpret_cast<const char*>(&count), sizeof(count));
        file.close();
        cout << "File saved successfully!\n";
    }

    delete s1;
    delete s2;
    delete s3;
    delete s4;
}

// MAIN MENU

int main() {
    cout << "========================================\n";
    cout << "STUDENT GRADES SYSTEM\n";
    cout << "Tasks 1-11\n";
    cout << "========================================\n";

    int choice;
    do {
        cout << "\n----------------------------------------\n";
        cout << "1  - Task 1 (Dynamic array)\n";
        cout << "2  - Task 2 (Vector of vectors)\n";
        cout << "3  - Task 3 (Sorting with lambda)\n";
        cout << "4  - Task 4 (Filtering remove_if)\n";
        cout << "5  - Task 5 (Binary file + pragma pack)\n";
        cout << "6  - Task 6 (Student class, explicit)\n";
        cout << "7  - Task 7 (Inheritance Person->Student/Teacher)\n";
        cout << "8  - Task 8 (Composition)\n";
        cout << "9  - Task 9 (Aggregation)\n";
        cout << "10 - Task 10 (inline method)\n";
        cout << "11 - Task 11 (Complete system)\n";
        cout << "0  - Exit\n";
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
        case 9: task9(); break;
        case 10: task10(); break;
        case 11: task11(); break;
        case 0: cout << "Exiting...\n"; break;
        default: cout << "Invalid choice!\n";
        }
    } while (choice != 0);

    return 0;
}