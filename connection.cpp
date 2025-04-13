#include "connection.h"

Connection::Connection(unsigned short port, uint32_t v):
    sock(socket(AF_INET, SOCK_STREAM, 0)), // TCP сокет
    serv_addr(new sockaddr_in), // пустая адресная структура сервера
    client_addr(new sockaddr_in) // пустая адресная структура клиента
{
    serv_v = v;//установка значения версии для привественного сообщения

    //проверка создания сокета
    if (sock == -1)
        throw std::system_error(errno, std::generic_category(), "Socket error");
    //установка параметров для сокета: возможность исп. сокета сразу после закрытия сервера
    int on = 1;
    int rc = setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);
    if (rc == -1 )
        throw std::system_error(errno, std::generic_category(), "Socket setopt error");
    //установка параметров для сокета: настройка таймаута для снятия блокировки

    struct timeval timeout {
        180, 0
    };
    rc = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    if (rc == -1 )
        throw std::system_error(errno, std::generic_category(), "Socket setopt error");
    //заполнение адресной структуры сервера
    serv_addr->sin_family = AF_INET; // всегда так
    serv_addr->sin_port = htons(port); // конкретное значение
    serv_addr->sin_addr.s_addr = inet_addr("127.0.0.1"); //конкретное значение
    //Привязка сокета к адресу сервера
    rc = bind(sock, reinterpret_cast<const sockaddr*>(serv_addr.get()), sizeof(sockaddr_in));
    if (rc == -1 )
        throw std::system_error(errno, std::generic_category(), "Bind error");

}
void Connection::connect(Send_recv& clc, Auth& ath, std::shared_ptr<Error> er)
{

    //режим ожидания соединения для сокета
    if (listen(sock, queue_len)==-1) {
        er->critic_set();
        throw std::system_error(errno, std::generic_category(), "Listen error");
    }
    socklen_t len = sizeof (sockaddr_in);
    //бесконечный цикл обработки входящих соединений
    while(1) {
        int work_sock = accept(sock, reinterpret_cast<sockaddr*>(client_addr.get()), &len);
        //
        if (work_sock == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::clog << "log: Accept: время ожидания клиентов истекло" << std::endl;
                //throw std::system_error(errno, std::generic_category(), "The waiting time expired");
                break;
            } else if (errno == ECONNABORTED) {
                std::clog << "log: Accept: клиент прервал соединение до завершения сеанса" << std::endl;
            } else {
                std::clog << "Accept error: " << strerror(errno) << std::endl;
            }
            continue;
        }
        //обработка случая, если подключится четвертый клиент
        if (thread_count.load() >= 3) {
            std::string rjct_msg = "Server busy. Try again later.";
            send(work_sock, rjct_msg.c_str(), rjct_msg.size(), 0);
            close(work_sock);
            std::clog << "log: Отклонено соединение с клиентом — превышен лимит потоков" << std::endl;
            continue;
        }

        thread_count++;
        std::thread(&Connection::set_thread, this, work_sock, std::ref(clc), std::ref(ath), er).detach();//this для передачи объекта класса Connection
        // чтобы вызвать метод set_thread


    }
}
void Connection::set_thread(int work_sock, Send_recv& clc, Auth& ath, std::shared_ptr<Error> er)
{

    std::string ip_addr = "unknown";  // Защита IP-адреса от изменений, пока поток его не использует
    sockaddr_in client_address;//копия адресной структуры, так как client_addr содержится адрес основног потока, занятого функцией accept()
    socklen_t addr_len = sizeof(client_address);
    getpeername(work_sock, reinterpret_cast<sockaddr*>(&client_address), &addr_len);//уточнение адреса, привязанного к work_sock(),
    //заполнение структуры алресом и портом

    ip_addr = inet_ntoa(client_address.sin_addr); //в sin_addr содержится правильный адрес клиента
    std::clog << "log: Соединение установлено с " << ip_addr <<std::endl;
    er->regular_log("Соединение установлено с " + ip_addr, "N/A", 0);
    std::clog << "log: Активных подключений: " << thread_count.load() <<std::endl;
    //отправка приветственного сообщения с номером версии клиенту
    std::string priv_msg = "            СЕРВЕР ПРИЕМА ФАЙЛОВ(версия " + std::to_string(serv_v) + ")            ";
    send(work_sock, priv_msg.c_str(), priv_msg.size(), 0);

    try {
        ath(work_sock, er); //передача сокета модулю аутентификации и выполнении идент. и аутент.
        bool flag = clc(work_sock, ath.get_status(), ath.get_id(), er);	//передача сокета модулю вычислений и расчет(оператор возвр false, если клиент завершил соед. сам)
        if (!flag) {
            std::clog << "log: Клиент " << ip_addr << " завершил соединение с помощью exit" << std::endl;
            er->regular_log("Клиент " + ip_addr + " завершил соединение с помощью exit", "N/A", 0);
        }

    } catch (const std::exception& ex) {
        std::clog << "log: Ошибка при при обработке клиента " << ip_addr << ": " << ex.what() << std::endl;
        er->error_log("Ошибка при обработке клиента " + ip_addr + ":" + ex.what());
    } catch (...) {
        std::clog << "log: Неизвестное ошибка от клиента " << ip_addr << std::endl;
        er->error_log("Неизвестное ошибка от клиента " + ip_addr);
    }

    std::clog << "log: Соединение закрыто c " << ip_addr <<std::endl;
    er->regular_log("Соединение закрыто c " + ip_addr, "N/A", 0);
    close(work_sock);
    thread_count--;
}
