#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

using boost::asio::ip::tcp;

std::mutex cout_mutex;

void safe_print(const std::string& s) {
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << s << std::endl;
}

int main() {
    try {
        boost::asio::io_context io;

        auto work = boost::asio::make_work_guard(io);

        std::vector<std::thread> pool;
        for (int i = 0; i < 4; ++i) {
            pool.emplace_back([&io]() { io.run(); });
        }

        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 12345));
        safe_print("Сервер запущен...");

        while (true) {
            tcp::socket socket(io);
            acceptor.accept(socket);

            boost::asio::post(io, [sock = std::make_shared<tcp::socket>(
                std::move(socket))]() {
                    auto strand = boost::asio::make_strand(sock->get_executor());

                    try {
                        while (true) {
                            boost::asio::streambuf buffer;
                            boost::asio::read_until(*sock, buffer, '\n');

                            std::istream input(&buffer);
                            std::string message;
                            std::getline(input, message);

                            safe_print("Получено: " + message);

                            if (message.rfind("timer", 0) == 0) {
                                std::istringstream iss(message);
                                std::string cmd;
                                int seconds;
                                iss >> cmd >> seconds;

                                std::string response =
                                    "Установлен таймер на " + std::to_string(seconds) + "\n";

                                boost::asio::post(strand, [sock, response]() {
                                    boost::asio::write(*sock, boost::asio::buffer(response));
                                    });

                                auto timer = std::make_shared<boost::asio::steady_timer>(
                                    sock->get_executor(), boost::asio::chrono::seconds(seconds));

                                timer->async_wait(boost::asio::bind_executor(
                                    strand,
                                    [sock, timer, seconds](const boost::system::error_code& ec) {
                                        if (!ec) {
                                            std::string msg =
                                                "Истек таймер на " + std::to_string(seconds) + "!\n";

                                            boost::asio::write(*sock, boost::asio::buffer(msg));

                                            std::lock_guard<std::mutex> lock(cout_mutex);
                                            std::cout << "Отправлено: " << msg;
                                        }
                                    }));
                            }
                        }
                    }
                    catch (...) {
                        safe_print("Клиент отключился");
                    }
                });
        }

        for (auto& t : pool)
            t.join();

    }
    catch (std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
    }
}