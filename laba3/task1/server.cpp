
#include <boost/asio.hpp>
#include <boost/locale.hpp>
#include <iostream>
#include <string>

using boost::asio::ip::tcp;

std::string to_upper(const std::string& input) {
    static boost::locale::generator gen;
    static std::locale loc = gen("");
    return boost::locale::to_upper(input, loc);
}

int main() {
    try {
        boost::asio::io_context io;

        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 12345));
        std::cout << "Сервер запущен на порту 12345...\n";

        while (true) {
            tcp::socket socket(io);
            acceptor.accept(socket);

            std::cout << "Клиент подключен: " << socket.remote_endpoint()
                << std::endl;

            while (true) {
                boost::asio::streambuf buffer;

                boost::asio::read_until(socket, buffer, '\n');

                std::istream input(&buffer);
                std::string message;
                std::getline(input, message);

                std::cout << "Получено: " << message << std::endl;

                std::string upper = to_upper(message);
                std::string response =
                    std::to_string(message.size()) + ": " + upper + "\n";

                boost::asio::write(socket, boost::asio::buffer(response));

                std::cout << "Отправлено: " << response << std::endl;
            }
        }

    }
    catch (std::exception& e) {
        std::cerr << "Ошибка сервера: " << e.what() << std::endl;
    }
}