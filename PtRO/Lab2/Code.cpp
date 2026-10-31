// Lab 2
// Synchronizing threads with events (condition variables) and futures
//
// Build: g++ -std=c++17 -Wall -pthread Code.cpp -o lab2
// Run:   ./lab2      - shows a menu
//        ./lab2 N    - runs task 1.2.N directly (N from 1 to 7)

#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdlib>
#include <deque>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

using namespace std;

// checks if a number is prime
bool isPrime(long long n) {
    if (n < 2) {
        return false;
    }
    if (n == 2) {
        return true;
    }
    if (n % 2 == 0) {
        return false;
    }
    for (long long d = 3; d * d <= n; d += 2) {
        if (n % d == 0) {
            return false;
        }
    }
    return true;
}

// returns the n-th prime number (1 -> 2, 2 -> 3, 3 -> 5, ...)
long long nthPrime(long long n) {
    long long count = 0;
    long long number = 1;
    while (count < n) {
        number++;
        if (isPrime(number)) {
            count++;
        }
    }
    return number;
}


// ---------------- Task 1.2.1 ----------------
// global flag + unique_lock + sleep_for
namespace task1 {

queue<long long> numbers;
bool dataReady = false;
mutex mtx;

void DataPreparation() {
    cout << "Enter integers (type a letter, for example q, to stop):" << endl;
    long long x;
    while (cin >> x) {
        unique_lock<mutex> lock(mtx);
        numbers.push(x);
    }

    unique_lock<mutex> lock(mtx);
    dataReady = true;
}

void DataProcessing() {
    while (true) {
        unique_lock<mutex> lock(mtx);
        if (dataReady == true) {
            cout << "Prime numbers from the queue: ";
            while (!numbers.empty()) {
                long long value = numbers.front();
                numbers.pop();
                if (isPrime(value)) {
                    cout << value << " ";
                }
            }
            cout << endl;
            return;
        }
        // unlock before sleeping so the other thread can add numbers
        lock.unlock();
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

void run() {
    thread preparation(DataPreparation);
    preparation.detach();

    thread processing(DataProcessing);
    processing.join();
}

}


// ---------------- Task 1.2.2 ----------------
// three Waits threads + Awake thread with notify_all
namespace task2 {

mutex mtx;
condition_variable cv;
int i = 0;

void Waits(int id) {
    unique_lock<mutex> lock(mtx);
    cout << "Waits " << id << ": waiting..." << endl;
    cv.wait(lock, []() { return i == 1; });
    cout << "Waits " << id << ": done waiting" << endl;
}

void Awake() {
    this_thread::sleep_for(chrono::milliseconds(100));
    {
        lock_guard<mutex> lock(mtx);
        cout << "Awake: notifying the other threads" << endl;
    }
    // first notify: i is still 0, so the threads go back to waiting
    cv.notify_all();

    this_thread::sleep_for(chrono::milliseconds(100));
    {
        lock_guard<mutex> lock(mtx);
        i = 1;
        cout << "Awake: notifying again" << endl;
    }
    // second notify: now i == 1, so the threads finish
    cv.notify_all();
}

void run() {
    thread w1(Waits, 1);
    thread w2(Waits, 2);
    thread w3(Waits, 3);
    thread awake(Awake);

    awake.join();
    w1.join();
    w2.join();
    w3.join();
}

}


// ---------------- Task 1.2.3 ----------------
// three threads + Notify thread with notify_one
namespace task3 {

mutex mtx;
condition_variable cv;
int i = 0;

void ThreadN(int id) {
    unique_lock<mutex> lock(mtx);
    cout << "Thread" << id << ": waiting for a signal" << endl;
    while (i != 1) {
        cv.wait(lock);
    }
    cout << "Message from thread " << id << endl;
}

void Notify() {
    this_thread::sleep_for(chrono::milliseconds(100));
    {
        lock_guard<mutex> lock(mtx);
        i = 1;
        cout << "Notify: waking up one thread" << endl;
    }
    cv.notify_one();
}

void run() {
    for (int id = 1; id <= 3; id++) {
        thread t(ThreadN, id);
        // two of the three threads will never wake up, so we detach them
        t.detach();
    }

    thread notify(Notify);
    notify.join();

    // give the woken thread some time to print its message
    this_thread::sleep_for(chrono::milliseconds(300));
}

}


// ---------------- Task 1.2.4 ----------------
// condition variable instead of a flag (condition: queue is not empty)
namespace task4 {

struct Item {
    long long value;
    bool isEnd; // true means the input is finished
};

queue<Item> items;
mutex mtx;
condition_variable cv;

void DataPreparation() {
    cout << "Enter integers (type a letter, for example q, to stop):" << endl;
    long long x;
    while (cin >> x) {
        {
            unique_lock<mutex> lock(mtx);
            items.push({x, false});
        }
        cv.notify_one();
    }

    {
        unique_lock<mutex> lock(mtx);
        items.push({0, true});
    }
    cv.notify_one();
}

void DataProcessing() {
    vector<long long> primes;
    while (true) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, []() { return !items.empty(); });
        Item item = items.front();
        items.pop();
        lock.unlock();

        if (item.isEnd) {
            break;
        }
        if (isPrime(item.value)) {
            primes.push_back(item.value);
        }
    }

    cout << "Prime numbers from the queue: ";
    for (size_t k = 0; k < primes.size(); k++) {
        cout << primes[k] << " ";
    }
    cout << endl;
}

void run() {
    thread preparation(DataPreparation);
    preparation.detach();

    thread processing(DataProcessing);
    processing.join();
}

}


// ---------------- Task 1.2.5 ----------------
// std::async with launch::deferred and launch::async
namespace task5 {

void run() {
    cout << "Launch mode: 1 - launch::deferred, 2 - launch::async: ";
    int mode;
    cin >> mode;

    launch policy;
    if (mode == 1) {
        policy = launch::deferred;
    } else {
        policy = launch::async;
    }

    cout << "Enter n (which prime number to find): ";
    long long n;
    cin >> n;
    if (n < 1) {
        cout << "n must be >= 1" << endl;
        return;
    }

    auto start = chrono::steady_clock::now();
    future<long long> result = async(policy, nthPrime, n);

    // in async mode the prime is being calculated while we talk to the user
    cout << "Choose a function of n: 1 - sqrt, 2 - sin, 3 - ln: ";
    int f;
    cin >> f;
    double x = (double)n;
    if (f == 1) {
        cout << "sqrt(" << n << ") = " << sqrt(x) << endl;
    } else if (f == 2) {
        cout << "sin(" << n << ") = " << sin(x) << endl;
    } else if (f == 3) {
        cout << "ln(" << n << ") = " << log(x) << endl;
    } else {
        cout << "Unknown function" << endl;
    }

    auto waitStart = chrono::steady_clock::now();
    long long prime = result.get();
    auto end = chrono::steady_clock::now();

    long long waitTime = chrono::duration_cast<chrono::milliseconds>(end - waitStart).count();
    long long totalTime = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "Prime number #" << n << ": " << prime << endl;
    cout << "Time spent waiting in get(): " << waitTime << " ms" << endl;
    cout << "Total time since launch: " << totalTime << " ms" << endl;
}

}


// ---------------- Task 1.2.6 ----------------
// two deques + packaged_task + separate worker thread
namespace task6 {

deque<packaged_task<long long()>> tasks; // deque 1: tasks
deque<long long> numbers;                // deque 2: the n values
mutex mtx;                               // protects both deques and finished
condition_variable cv;
bool finished = false;
mutex printMtx;                          // so the output of threads doesn't mix

void Worker() {
    while (true) {
        packaged_task<long long()> task;
        long long n;

        {
            unique_lock<mutex> lock(mtx);
            while (tasks.empty() && !finished) {
                cv.wait(lock);
            }
            if (tasks.empty()) {
                // finished and nothing left to do
                return;
            }
            task = move(tasks.front());
            tasks.pop_front();
            n = numbers.front();
            numbers.pop_front();
        }

        {
            lock_guard<mutex> lock(printMtx);
            cout << "[Worker] calculating prime number #" << n << "..." << endl;
        }

        task();

        {
            lock_guard<mutex> lock(printMtx);
            cout << "[Worker] prime number #" << n << " is ready" << endl;
        }
    }
}

void run() {
    thread worker(Worker);

    vector<long long> inputs;
    vector<future<long long>> results;

    while (true) {
        {
            lock_guard<mutex> lock(printMtx);
            cout << "Enter n (0 or a letter to stop): " << flush;
        }
        long long n;
        if (!(cin >> n) || n <= 0) {
            break;
        }

        packaged_task<long long()> task([n]() { return nthPrime(n); });
        inputs.push_back(n);
        results.push_back(task.get_future());

        {
            unique_lock<mutex> lock(mtx);
            tasks.push_back(move(task));
            numbers.push_back(n);
        }
        cv.notify_one();
    }

    {
        unique_lock<mutex> lock(mtx);
        finished = true;
    }
    cv.notify_one();

    // the worker finishes the remaining tasks and then exits
    worker.join();

    cout << endl << "=== Results ===" << endl;
    for (size_t k = 0; k < results.size(); k++) {
        cout << "Prime number #" << inputs[k] << ": " << results[k].get() << endl;
    }
}

}


// ---------------- Task 1.2.7 ----------------
// promise / future, no global variables
// (threads return void, everything is passed through arguments)
namespace task7 {

void FirstThread(long long n, promise<long long> primeN, promise<bool> startSignal, promise<long long> prime10N) {
    // 1) send the n-th prime to the main thread
    primeN.set_value(nthPrime(n));
    // 2) tell the second thread it can start
    startSignal.set_value(true);
    // 3) send the (n*10)-th prime to the main thread
    prime10N.set_value(nthPrime(n * 10));
}

void SecondThread(long long n, future<bool> startSignal) {
    startSignal.get(); // wait for the signal from FirstThread
    this_thread::sleep_for(chrono::seconds(2));
    cout << "Square root of " << n << " = " << sqrt((double)n) << endl;
}

void run() {
    cout << "Enter n (which prime number to find): ";
    long long n;
    if (!(cin >> n) || n < 1) {
        cout << "Invalid n" << endl;
        return;
    }

    promise<long long> primeNPromise;
    promise<long long> prime10NPromise;
    promise<bool> startPromise;

    future<long long> primeNFuture = primeNPromise.get_future();
    future<long long> prime10NFuture = prime10NPromise.get_future();
    future<bool> startFuture = startPromise.get_future();

    thread thr1(FirstThread, n, move(primeNPromise), move(startPromise), move(prime10NPromise));
    thread thr2(SecondThread, n, move(startFuture));

    cout << "Prime number #" << n << ": " << primeNFuture.get() << endl;
    cout << "Prime number #" << n * 10 << ": " << prime10NFuture.get() << endl;

    thr1.join();
    thr2.join();
}

}


int main(int argc, char* argv[]) {
    int choice = 0;

    if (argc > 1) {
        choice = atoi(argv[1]);
    } else {
        cout << "Lab 2. Choose a task:" << endl;
        cout << "  1 - 1.2.1 (flag + sleep_for)" << endl;
        cout << "  2 - 1.2.2 (notify_all, three Waits threads)" << endl;
        cout << "  3 - 1.2.3 (notify_one, three threads)" << endl;
        cout << "  4 - 1.2.4 (condition variable instead of a flag)" << endl;
        cout << "  5 - 1.2.5 (std::async: deferred / async)" << endl;
        cout << "  6 - 1.2.6 (packaged_task + deques + worker thread)" << endl;
        cout << "  7 - 1.2.7 (promise / future)" << endl;
        cout << "Your choice: ";
        cin >> choice;
    }

    if (choice == 1) {
        task1::run();
    } else if (choice == 2) {
        task2::run();
    } else if (choice == 3) {
        task3::run();
    } else if (choice == 4) {
        task4::run();
    } else if (choice == 5) {
        task5::run();
    } else if (choice == 6) {
        task6::run();
    } else if (choice == 7) {
        task7::run();
    } else {
        cout << "Unknown task number" << endl;
        return 1;
    }

    return 0;
}
