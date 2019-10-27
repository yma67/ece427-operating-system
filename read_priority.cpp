#include <iostream>
#include <string>
#include <mutex>
#include <thread>
#include <vector>
#include <utility>
#include <cstdlib>
#include <chrono>
#include <condition_variable>

static int var = 0;

struct reader_writer {
private:
    int read_count, write_count;
    std::mutex lock;
    std::condition_variable read_queue, write_queue;
    
public:
    reader_writer() : read_count(0), write_count(0) {}
    
    void read_lock() {
        std::unique_lock<std::mutex> lc(lock);
        read_count += 1;
        read_queue.wait(lc, [this]() -> bool {
            return !(write_count > 0);
        });
        read_queue.notify_one();
    }
    
    void read_unlock() {
        std::unique_lock<std::mutex> lc(lock);
        read_count -= 1;
        if (read_count == 0)
            write_queue.notify_one();
    }
    
    void write_lock() {
        std::unique_lock<std::mutex> lc(lock);
        write_queue.wait(lc, [this]() -> bool {
            return !(read_count > 0 || write_count > 0);
        });
        std::cout << "w" << std::endl;
        write_count += 1;
    }
    
    void write_unlock() {
        std::unique_lock<std::mutex> lc(lock);
        write_count -= 1;
        if (read_count > 0)
            read_queue.notify_one();
        else
            write_queue.notify_one();
    }
};

static reader_writer rw;

void read(int ntry) {
    for (int i = 0; i < ntry; i++) {
        rw.read_lock();
        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 100));
        std::cout << var << std::endl;
        rw.read_unlock();
    }
}

void write(int ntry) {
    for (int i = 0; i < ntry; i++) {
        rw.write_lock();
        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 100));
        var += 10;
        rw.write_unlock();
    }
}

int main() {
    std::vector<std::thread> rpool, wpool;
    int rtry = 60;
    int wtry = 30;
    int& rrtry = rtry;
    int& rwtry = wtry;
    for (int i = 0; i < 500; i++) 
        rpool.push_back(std::thread (read, rtry));
    for (int i = 0; i < 10; i++) 
        wpool.push_back(std::thread (write, wtry));
    for (auto& tr: rpool)
        tr.join();
    for (auto& tw: wpool)
        tw.join();
    std::cout << var << std::endl;
    return 0;
}

