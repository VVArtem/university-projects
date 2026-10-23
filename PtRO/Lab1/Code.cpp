//Завдання 1.2.1 — Базовий запуск потоків

#include <iostream>
#include <thread>

void Thread1() {
    std::cout << 1 << std::endl;
}

void Thread2() {
    std::cout << 2 << std::endl;
}

int main() {
    std::thread t1(Thread1);
    std::thread t2(Thread2);
    std::cout << "Main thread finished" << std::endl;
    return 0;
}

//Завдання 1.2.2 — Використання методу detach

#include <iostream>
#include <thread>

void Thread1() {
    std::cout << 1 << std::endl;
}

void Thread2() {
    std::cout << 2 << std::endl;
}

int main() {
    std::thread t1(Thread1);
    std::thread t2(Thread2);
    t1.detach();
    t2.detach();
    std::cout << "Main thread finished" << std::endl;
    return 0;
}

//Завдання 1.2.3 — Робота з глобальним списком (без синхронізації)

std::list<int> l;

void AddToList(int value) {
    for (int i = 0; i < 10; ++i) {
        l.push_back(value + i);
        std::cout << "Added: " << value + i << std::endl;
    }
}

void ListContains(int value) {
    for (int i = 0; i < 10; ++i) {
        bool found = std::find(l.begin(), l.end(), value) != l.end();
        std::cout << "Value " << value << (found ? " contains" : " does not contain") << " in the list" << std::endl;
    }
}

int main() {
    int addValue = 5;
    int checkValue = 7;

    std::thread t1(AddToList, addValue);
    std::thread t2(ListContains, checkValue);

    t1.join();
    t2.join();

    return 0;
}

//Завдання 1.2.4 — Синхронізація за допомогою м'ютексів

std::list<int> l;
std::mutex listMutex;

void AddToList(int value) {
    for (int i = 0; i < 10; ++i) {
        listMutex.lock();
        l.push_back(value + i);
        std::cout << "Added: " << value + i << std::endl;
        listMutex.unlock();
    }
}

void ListContains(int value) {
    for (int i = 0; i < 10; ++i) {
        listMutex.lock();
        bool found = std::find(l.begin(), l.end(), value) != l.end();
        std::cout << "Value " << value << (found ? " contains" : " does not contain") << " in the list" << std::endl;
        listMutex.unlock();
    }
}

int main() {
    int addValue = 5;
    int checkValue = 7;

    std::thread t1(AddToList, addValue);
    std::thread t2(ListContains, checkValue);

    t1.join();
    t2.join();

    return 0;
}

//Завдання 1.2.5 — Застосування шаблону lock_guard

std::list<int> l;
std::mutex listMutex;

void AddToList(int value) {
    std::lock_guard<std::mutex> guard(listMutex);
    l.push_back(value);
    std::cout << "Added: " << value << std::endl;
}

void ListContains(int value) {
    std::lock_guard<std::mutex> guard(listMutex);
    bool found = std::find(l.begin(), l.end(), value) != l.end();
    std::cout << "Value " << value << (found ? " contains" : " does not contain") << " in the list" << std::endl;
}

int main() {
    int addValue = 1;
    int checkValue = 5;

    for (int i = 0; i < 10; ++i) {
        std::thread t1(AddToList, addValue + i);
        std::thread t2(ListContains, checkValue);
        t1.detach();
        t2.detach();
    }

    std::this_thread::sleep_for(std::chrono::seconds(1)); // очікування завершення відкріплених потоків
    return 0;
}

//Завдання 1.2.6 — Класи someData та exchangePerson

class someData {
public:
    std::string firstName;
    std::string lastName;
    std::string address;
    int age;

    someData(std::string fn = "", std::string ln = "", std::string addr = "", int a = 0)
        : firstName(fn), lastName(ln), address(addr), age(a) {}

    void Print() const {
        std::cout << "Name: " << firstName << ", Last name: " << lastName
                   << ", Address: " << address << ", Age: " << age << std::endl;
    }
};

class exchangePerson {
public:
    someData data;
    std::mutex m;

    static void JohnDoe(exchangePerson& person) {
        person.data = someData("John", "Doe", "Unknown", 120);
    }

    static void JacobSmith(exchangePerson& person) {
        person.data = someData("Jacob", "Smith", "Known", 1);
    }

    static void Swap(exchangePerson& p1, exchangePerson& p2) {
        if (&p1 == &p2) {
            std::cout << "Cannot swap an object with itself." << std::endl;
            return;
        }

        std::cout << "Before swap:" << std::endl;
        std::cout << "Person 1: "; p1.data.Print();
        std::cout << "Person 2: "; p2.data.Print();

        std::lock(p1.m, p2.m);
        std::lock_guard<std::mutex> lock1(p1.m, std::adopt_lock);
        std::lock_guard<std::mutex> lock2(p2.m, std::adopt_lock);

        std::swap(p1.data, p2.data);

        std::cout << "After swap:" << std::endl;
        std::cout << "Person 1: "; p1.data.Print();
        std::cout << "Person 2: "; p2.data.Print();
    }
};

int main() {
    exchangePerson person1;
    exchangePerson person2;

    std::thread t1(exchangePerson::JohnDoe, std::ref(person1));
    std::thread t2(exchangePerson::JacobSmith, std::ref(person2));
    t1.join();
    t2.join();

    exchangePerson::Swap(person1, person2);

    return 0;
}
