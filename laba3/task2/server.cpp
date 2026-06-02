#include <algorithm>
#include <boost/asio.hpp>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

using boost::asio::ip::tcp;

int find_max(const std::string& input) {
    std::istringstream iss(input);
    std::vector<int> nums;
    int x;

    while (iss >> x) {
        nums.push_back(x);
    }

    if (nums.empty())
        return 0;

    return *std::max_element(nums.begin(), nums.end());
}

int main() {
    try {
        boost::asio::io_context io;
        auto work = boost::asio::make_work_guard(io);
        std::vector<std::thread> pool;
        const int THREADS = 4;

        for (int i = 0; i < THREADS; ++i) {
            pool.emplace_back([&io]() { io.run(); });
        }

        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 12345));
        std::cout << "Сервер запущен...\n";

        while (true) {
            tcp::socket socket(io);
            acceptor.accept(socket);

            std::cout << "Клиент подключен\n";

            boost::asio::post(io, [sock = std::move(socket)]() mutable {
                try {
                    while (true) {
                        boost::asio::streambuf buffer;

                        boost::asio::read_until(sock, buffer, '\n');

                        std::istream input(&buffer);
                        std::string message;
                        std::getline(input, message);

                        int max_val = find_max(message);

                        std::string response =
                            "Максимум: " + std::to_string(max_val) + "\n";

                        boost::asio::write(sock, boost::asio::buffer(response));
                    }
                }
                catch (...) {
                    std::cout << "Клиент отключился\n";
                }
                });
        }

        for (auto& t : pool)
            t.join();

    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}