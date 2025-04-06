#include "AyncLogger.hpp"

int main() {

    try {
        AsyncLogger logger("log.txt");

        logger.log("Main Thread : Hello, World!");
        logger.log("Main Thread : This is a test log message.");
        logger.log("Main Thread : Spawning new thread.");
        // std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::thread t1([&logger]() {
            for (int i = 0; i < 10000; ++i) {
                logger.log("Thread 1: Log message " + std::to_string(i));
            }
        });
        logger.log("Main Thread : This is an asynchronous log message.");
        logger.log("Main Thread : Logging is asynchronous.");
        for (int i = 0; i < 10000; ++i) {
            logger.log("Main Thread: Log message " + std::to_string(i));
        }

        t1.join();

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception occurred." << std::endl;
        return 1;
    }

    return 0;
}