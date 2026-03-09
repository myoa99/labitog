#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <regex>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>

using namespace std;

struct Employee {
    string fio;
    string birthdate; // YYYY-MM-DD
    double salary;
    int age;
};

bool isLeap(int y) {
    return (y % 400 == 0) || (y % 4 == 0 && y % 100 != 0);
}

bool validDate(string s) {
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;

    int y = stoi(s.substr(0, 4));
    int m = stoi(s.substr(5, 2));
    int d = stoi(s.substr(8, 2));

    if (m < 1 || m > 12 || d < 1) return false;

    int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (m == 2 && isLeap(y)) days[1] = 29;

    return d <= days[m - 1];
}

int getAge(string birthdate) {
    int y = stoi(birthdate.substr(0, 4));
    int m = stoi(birthdate.substr(5, 2));
    int d = stoi(birthdate.substr(8, 2));

    time_t t = time(0);
    tm now = *localtime(&t);

    int age = (now.tm_year + 1900) - y;
    if ((now.tm_mon + 1 < m) || ((now.tm_mon + 1 == m) && now.tm_mday < d)) age--;

    return age;
}

string readFile(string filename) {
    ifstream f(filename);
    if (!f) throw runtime_error("Не удалось открыть файл");
    return string((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
}

vector<Employee> loadEmployees(string filename) {
    string text = readFile(filename);

    regex objRe("\\{[^\\{\\}]*\\}");
    regex fioRe("\"fio\"\\s*:\\s*\"([^\"]+)\"");
    regex birthRe("\"birthdate\"\\s*:\\s*\"([0-9]{4}-[0-9]{2}-[0-9]{2})\"");
    regex salRe("\"salary\"\\s*:\\s*(-?[0-9]+(\\.[0-9]+)?)");

    vector<Employee> v;

    for (sregex_iterator it(text.begin(), text.end(), objRe), end; it != end; ++it) {
        string obj = it->str();
        smatch m1, m2, m3;

        if (!regex_search(obj, m1, fioRe)) throw runtime_error("Нет поля fio");
        if (!regex_search(obj, m2, birthRe)) throw runtime_error("Нет поля birthdate");
        if (!regex_search(obj, m3, salRe)) throw runtime_error("Нет поля salary");

        string fio = m1[1];
        string birthdate = m2[1];
        double salary = stod(m3[1]);

        if (!validDate(birthdate)) throw runtime_error("Неверная дата: " + birthdate);
        if (salary < 0) throw runtime_error("Зарплата не может быть отрицательной");

        Employee e;
        e.fio = fio;
        e.birthdate = birthdate;
        e.salary = salary;
        e.age = getAge(birthdate);

        v.push_back(e);
    }

    if (v.empty()) throw runtime_error("Нет данных в JSON");
    return v;
}

void sortEmployees(vector<Employee>& v, string field, bool desc) {
    sort(v.begin(), v.end(), [&](Employee a, Employee b) {
        bool res = false;

        if (field == "fio") res = a.fio < b.fio;
        else if (field == "birthdate") res = a.birthdate < b.birthdate; // YYYY-MM-DD сравнивается правильно как строка
        else if (field == "salary") res = a.salary < b.salary;
        else if (field == "age") res = a.age < b.age;
        else res = a.fio < b.fio;

        return desc ? !res : res;
    });
}

vector<Employee> filterEmployees(vector<Employee> v, double minSalary, double maxSalary, int minAge, int maxAge) {
    vector<Employee> res;
    for (auto e : v) {
        if (e.salary >= minSalary && e.salary <= maxSalary &&
            e.age >= minAge && e.age <= maxAge) {
            res.push_back(e);
        }
    }
    return res;
}

double averageSalary(vector<Employee> v) {
    if (v.empty()) return 0;
    double sum = 0;
    for (auto e : v) sum += e.salary;
    return sum / v.size();
}

double medianSalary(vector<Employee> v) {
    if (v.empty()) return 0;
    sort(v.begin(), v.end(), [](Employee a, Employee b) {
        return a.salary < b.salary;
    });

    int n = v.size();
    if (n % 2 == 1) return v[n / 2].salary;
    return (v[n / 2 - 1].salary + v[n / 2].salary) / 2.0;
}

void printEmployees(vector<Employee> v) {
    cout << left
         << setw(25) << "ФИО"
         << setw(12) << "Дата"
         << setw(10) << "Возраст"
         << setw(12) << "Зарплата" << "\n";

    cout << string(60, '-') << "\n";

    for (auto e : v) {
        cout << left
             << setw(25) << e.fio.substr(0, 24)
             << setw(12) << e.birthdate
             << setw(10) << e.age
             << fixed << setprecision(2) << e.salary << "\n";
    }

    cout << "\nСредняя зарплата: " << fixed << setprecision(2) << averageSalary(v) << "\n";
    cout << "Медиана зарплаты: " << fixed << setprecision(2) << medianSalary(v) << "\n";
}

void printHelp(string prog) {
    cout << "Использование:\n";
    cout << "  " << prog << " --input employees.json [опции]\n\n";
    cout << "Опции:\n";
    cout << "  --help\n";
    cout << "  --input FILE\n";
    cout << "  --sort fio|birthdate|salary|age\n";
    cout << "  --desc\n";
    cout << "  --min-salary N\n";
    cout << "  --max-salary N\n";
    cout << "  --min-age N\n";
    cout << "  --max-age N\n";
    cout << "  --bench\n\n";
    cout << "Пример:\n";
    cout << "  " << prog << " --input employees.json --sort birthdate --desc --min-salary 50000 --max-age 40\n";
}

int main(int argc, char* argv[]) {
    try {
        string input = "";
        string sortField = "fio";
        bool desc = false;
        bool bench = false;

        double minSalary = 0;
        double maxSalary = 1e18;
        int minAge = 0;
        int maxAge = 200;

        for (int i = 1; i < argc; i++) {
            string a = argv[i];

            if (a == "--help") {
                printHelp(argv[0]);
                return 0;
            } else if (a == "--input" && i + 1 < argc) {
                input = argv[++i];
            } else if (a == "--sort" && i + 1 < argc) {
                sortField = argv[++i];
            } else if (a == "--desc") {
                desc = true;
            } else if (a == "--min-salary" && i + 1 < argc) {
                minSalary = stod(argv[++i]);
            } else if (a == "--max-salary" && i + 1 < argc) {
                maxSalary = stod(argv[++i]);
            } else if (a == "--min-age" && i + 1 < argc) {
                minAge = stoi(argv[++i]);
            } else if (a == "--max-age" && i + 1 < argc) {
                maxAge = stoi(argv[++i]);
            } else if (a == "--bench") {
                bench = true;
            } else {
                throw runtime_error("Неизвестный аргумент: " + a);
            }
        }

        if (input == "") {
            printHelp(argv[0]);
            return 1;
        }

        vector<Employee> employees = loadEmployees(input);

        auto t1 = chrono::high_resolution_clock::now();
        sortEmployees(employees, sortField, desc);
        auto t2 = chrono::high_resolution_clock::now();

        auto t3 = chrono::high_resolution_clock::now();
        for (auto& e : employees) e.age = getAge(e.birthdate);
        auto t4 = chrono::high_resolution_clock::now();

        employees = filterEmployees(employees, minSalary, maxSalary, minAge, maxAge);

        printEmployees(employees);

        if (bench) {
            double sortMs = chrono::duration<double, milli>(t2 - t1).count();
            double ageMs = chrono::duration<double, milli>(t4 - t3).count();

            cout << "\n--- Bench ---\n";
            cout << "Сортировка: " << sortMs << " ms\n";
            cout << "Расчёт возрастов: " << ageMs << " ms\n";
            cout << "Узкое место: " << (sortMs > ageMs ? "сортировка" : "расчёт возрастов") << "\n";
        }
    }
    catch (exception& e) {
        cout << "Ошибка: " << e.what() << "\n";
        return 1;
    }

    return 0;
}