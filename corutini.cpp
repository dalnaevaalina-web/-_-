#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/awaitable_operators.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <random>
#include <atomic>
#include <variant>

using namespace boost::asio;
using namespace boost::asio::ip;
using namespace boost::asio::awaitable_operators;
using boost::system::error_code;

// ============================================================
// ЗАДАЧА 1: Эхо-сервер на корутинах
// ============================================================

awaitable<void> echo_session(tcp::socket sock) {
    char data[1024];
    try {
        for (;;) {
            auto [ec, n] = co_await sock.async_read_some(
                buffer(data, 1024),
                as_tuple(use_awaitable)
            );

            if (ec == error::eof) {
                std::cout << "Клиент отключился\n";
                break;
            }
            if (ec) {
                throw boost::system::system_error(ec);
            }

            co_await async_write(sock, buffer(data, n), use_awaitable);
            std::cout << "Эхо отправлено: " << std::string(data, n) << "\n";
        }
    }
    catch (const std::exception& e) {
        std::cout << "Ошибка сессии: " << e.what() << "\n";
    }
}

awaitable<void> echo_server(io_context& io, short port) {
    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), port));
    std::cout << "Эхо-сервер запущен на порту " << port << "\n";

    for (;;) {
        tcp::socket socket = co_await acceptor.async_accept(use_awaitable);
        std::cout << "Новый клиент подключился\n";
        co_spawn(io, echo_session(std::move(socket)), detached);
    }
}

void task1() {
    std::cout << "\n========== ЗАДАЧА 1: Эхо-сервер ==========\n";
    std::cout << "Запустите отдельно клиент (telnet или свой)\n";
    std::cout << "Для выхода нажмите Ctrl+C\n\n";

    io_context io;
    co_spawn(io, echo_server(io, 12345), detached);
    io.run();
}

// ============================================================
// ЗАДАЧА 2: Мультиплексирование двух источников данных
// ============================================================

awaitable<std::string> read_from(tcp::socket& socket, const std::string& name) {
    char data[256];
    auto [ec, n] = co_await socket.async_read_some(
        buffer(data, 256),
        as_tuple(use_awaitable)
    );

    if (ec) {
        co_return "[ОШИБКА] " + name + ": " + ec.message();
    }

    co_return "[" + name + "] " + std::string(data, n);
}

awaitable<void> multiplexer(tcp::socket& sock1, tcp::socket& sock2) {
    for (;;) {
        auto result = co_await(read_from(sock1, "SOCKET1") || read_from(sock2, "SOCKET2"));

        std::visit([](auto& val) {
            std::cout << val << std::endl;
            }, result);
    }
}

void task2() {
    std::cout << "\n========== ЗАДАЧА 2: Мультиплексирование ==========\n";
    std::cout << "Нужны два подключения к разным серверам\n";
    std::cout << "Для демонстрации создадим два локальных сокета\n\n";

    io_context io;

    // Создаём два тестовых сокета, подключённых к эхо-серверу
    tcp::socket sock1(io);
    tcp::socket sock2(io);

    tcp::resolver resolver(io);
    auto endpoints = resolver.resolve("127.0.0.1", "12345");

    try {
        connect(sock1, endpoints);
        connect(sock2, endpoints);

        std::cout << "Оба сокета подключены\n";

        // Запускаем мультиплексор
        co_spawn(io, multiplexer(sock1, sock2), detached);

        // Запускаем отправку тестовых сообщений
        std::thread sender([&]() {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::string msg1 = "Hello from client1\n";
            std::string msg2 = "Hello from client2\n";
            write(sock1, buffer(msg1));
            write(sock2, buffer(msg2));
            });
        sender.detach();

        io.run();
    }
    catch (const std::exception& e) {
        std::cout << "Ошибка: " << e.what() << "\n";
        std::cout << "Сначала запустите задачу 1 (эхо-сервер)\n";
    }
}

// ============================================================
// ЗАДАЧА 3: Банковский счёт с корутинами и strand
// ============================================================

class BankAccount {
private:
    int64_t balance_;
    io_context::strand& strand_;

public:
    BankAccount(io_context::strand& strand, int64_t initial_balance = 0)
        : balance_(initial_balance), strand_(strand) {
    }

    awaitable<void> async_deposit(int64_t amount) {
        co_await post(strand_, use_awaitable);
        balance_ += amount;
        std::cout << "Депозит: +" << amount << ", баланс: " << balance_ << "\n";
    }

    awaitable<void> async_withdraw(int64_t amount) {
        co_await post(strand_, use_awaitable);
        if (amount > balance_) {
            throw std::invalid_argument("Insufficient funds");
        }
        balance_ -= amount;
        std::cout << "Снятие: -" << amount << ", баланс: " << balance_ << "\n";
    }

    awaitable<int64_t> async_get_balance() {
        co_await post(strand_, use_awaitable);
        co_return balance_;
    }
};

awaitable<void> client_routine(BankAccount& account, int id, int num_transactions) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> amount_dist(1, 100);
    std::uniform_int_distribution<> type_dist(0, 1);

    for (int i = 0; i < num_transactions; ++i) {
        int64_t amount = amount_dist(gen);
        bool is_deposit = type_dist(gen) == 0;

        try {
            if (is_deposit) {
                co_await account.async_deposit(amount);
            }
            else {
                co_await account.async_withdraw(amount);
            }
        }
        catch (const std::invalid_argument& e) {
            std::cout << "Клиент " << id << ": " << e.what() << " (попытка снять " << amount << ")\n";
        }

        co_await steady_timer(co_await this_coro::executor, std::chrono::milliseconds(10));
    }

    std::cout << "Клиент " << id << " завершил операции\n";
}

void task3() {
    std::cout << "\n========== ЗАДАЧА 3: Банковский счёт ==========\n";

    const int NUM_THREADS = 4;
    const int NUM_CLIENTS = 10;
    const int TRANSACTIONS_PER_CLIENT = 20;

    io_context io;
    auto strand = io_context::strand(io.get_executor());

    BankAccount account(strand, 1000);

    std::atomic<int> finished_clients{ 0 };

    std::cout << "Начальный баланс: " << co_await account.async_get_balance() << "\n";

    // Запуск клиентов
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        co_spawn(io, [&account, i]() -> awaitable<void> {
            co_await client_routine(account, i, TRANSACTIONS_PER_CLIENT);
            finished_clients++;
            }, detached);
    }

    // Запуск потоков
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&io]() { io.run(); });
    }

    // Ожидание завершения всех клиентов
    while (finished_clients < NUM_CLIENTS) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    io.stop();

    for (auto& t : threads) {
        t.join();
    }

    int64_t final_balance = co_await account.async_get_balance();
    std::cout << "\nФинальный баланс: " << final_balance << "\n";
    std::cout << "Все клиенты завершили работу\n";
}

// ============================================================
// MAIN
// ============================================================

int main() {
    std::cout << "========================================\n";
    std::cout << "ЗАДАЧИ ПО КОРУТИНАМ\n";
    std::cout << "========================================\n";

    int choice;
    do {
        std::cout << "\n----------------------------------------\n";
        std::cout << "1 - Задача 1 (Эхо-сервер)\n";
        std::cout << "2 - Задача 2 (Мультиплексирование)\n";
        std::cout << "3 - Задача 3 (Банковский счёт)\n";
        std::cout << "0 - Выход\n";
        std::cout << "----------------------------------------\n";
        std::cout << "Ваш выбор: ";
        std::cin >> choice;

        switch (choice) {
        case 1:
            task1();
            break;
        case 2:
            task2();
            break;
        case 3:
            task3();
            break;
        case 0:
            std::cout << "Выход...\n";
            break;
        default:
            std::cout << "Неверный выбор!\n";
        }
    } while (choice != 0);

    return 0;
}