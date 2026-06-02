#include <boost/asio.hpp>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

using boost::asio::ip::tcp;

std::mutex cout_mutex;
const std::string prompt = "Введите строку: ";

void print_prompt() { std::cout << prompt << std::flush; }

void read_loop(tcp::socket& socket) {
    try {
        boost::asio::streambuf buffer;

        while (true) {
            boost::asio::read_until(socket, buffer, '\n');

            std::istream input(&buffer);
            std::string response;
            std::getline(input, response);

            std::lock_guard<std::mutex> lock(cout_mutex);

            std::cout << "\r\33[2K";

            std::cout << "Ответ сервера: " << response << std::endl;

            print_prompt();
        }
    }
    catch (...) {
    }
}

int main() {
    try {
        boost::asio::io_context io;

        tcp::socket socket(io);
        socket.connect({ boost::asio::ip::make_address("127.0.0.1"), 12345 });

        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Подключено к серверу.\n";
            print_prompt();
        }

        std::thread reader(read_loop, std::ref(socket));

        while (true) {
            std::string message;
            std::getline(std::cin, message);

            if (message == "exit")
                break;

            message += "\n";
            boost::asio::write(socket, boost::asio::buffer(message));

            std::lock_guard<std::mutex> lock(cout_mutex);
            print_prompt();
        }

        socket.close();
        reader.join();

    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}