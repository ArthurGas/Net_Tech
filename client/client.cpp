//C++
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <typeinfo>
#include <system_error>
#include <string>
#include <termios.h>
#include <unistd.h>

//UNIX & POSIX
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

//Crypto++
#include <cryptopp/cryptlib.h>
#include <cryptopp/hex.h>
#include <cryptopp/files.h>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>

using namespace CryptoPP;
int rc;
const size_t BUF_SIZE = 4096;
//функция заполнения базы 'имя - id"
std::map<std::string, std::string> base_read(std::string file_name)
{

    std::string sep=":";
    size_t pos;
    std::string buf, name, id;
    std::map<std::string, std::string> base;

    std::ifstream file(file_name.c_str());
    if (file.is_open()) {
        if(file.peek() == std::ifstream::traits_type::eof())
            throw std::length_error("base File Is Empty");
        while(getline(file, buf)) {
            while (buf.find(" ") < buf.size()) {
                buf.erase(buf.find(" "), 1);
            }
            pos = buf.find(sep);
            name = buf.substr(0, pos);
            id = buf.substr(pos+1, buf.size());
            base[name] = id;
        }
        file.close();
        return base;
    } else {
        throw std::system_error(errno, std::generic_category(), "base Read Error");
    }
}
std::string string_recv(int sock)
{
    int rc;
    int buflen = 1024;
    std::unique_ptr <char[]> buf(new char[buflen]);

    while (true) {
        rc = recv(sock, buf.get(), buflen, MSG_PEEK);
        if (rc == -1) {
            throw std::system_error(errno, std::generic_category(), "Recv string error");
        }
        if (rc < buflen)
            break;
        buflen *= 2;
        buf = std::unique_ptr<char[]>(new char[buflen]);
    }
    std::string res(buf.get(), rc);
    rc = recv(sock, nullptr, rc, MSG_TRUNC);
    if (rc == -1) {
        throw std::system_error(errno, std::generic_category(), "Clear bufer error");
    }
    res.resize(res.find_last_not_of("\n\r") + 1);
    return res;
}
//функция отправки имени файла
void send_name(int sock, const std::string& filename)
{
    uint32_t name_l = filename.size();
    uint32_t net_name_l = htonl(name_l); // переводим в сетевой порядок байтов
    // Сначала отправляем длину имени
    rc = send(sock, &net_name_l, sizeof(net_name_l), 0);
    if ( rc == -1 )
        throw std::system_error(errno, std::generic_category());

    // Затем отправляем само имя
    rc = send(sock, filename.c_str(), name_l, 0);
    if ( rc == -1 )
        throw std::system_error(errno, std::generic_category());
}
//функция отправки файла
void send_file(int sock, const std::string& ifile_name)
{
    std::string ifile_path="Files/"+ifile_name;
    //открытие файла для чтения
    std::ifstream file(ifile_path, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open file: " << ifile_path << "\n";
        throw std::system_error(errno, std::generic_category(), "Open file error");
    }

    std::streamsize file_size = file.tellg();
    file.seekg(0);
    std::cout<<"Отправка размера файла\n";
    // Отправка размера файла
    if (send(sock, &file_size, sizeof(file_size), 0) != sizeof(file_size)) {
        throw std::system_error(errno, std::generic_category(), "File size send error");

    }
    std::cout<<"Отправка файла\n";
    char buffer[BUF_SIZE];
    while (file) {
        file.read(buffer, BUF_SIZE);
        std::streamsize read_chars = file.gcount();
        //отправка содержимого файла по 4096 байт
        ssize_t sent = send(sock, buffer, read_chars, 0);
        if (sent != read_chars) {
            throw std::system_error(errno, std::generic_category(), "Bytes recv error");
        }
    }
    file.close();
}
std::vector<std::string> get_log(int sock)
{
    //вектор для приема строк лога
    std::vector<std::string> filtered_logs;
    //получение количества строк
    size_t log_count;
    int rc = recv(sock, &log_count, sizeof(log_count), 0);
    if (rc == -1)
        throw std::system_error(errno, std::generic_category(), "Filtered log size recv error");
    //цикл по получению длины строк, самих строк и добавлению в вектор
    for(size_t i=1; i<=log_count; i++) {
        size_t line_len=0;
        //прием длины строки
        int rc = recv(sock, &line_len, sizeof(line_len), 0);
        if (rc == -1) {
            throw std::system_error(errno, std::generic_category(), "Filename length recv error");
        }
        //создание буфера для получения строки
        std::unique_ptr<char> buffer(new char[line_len]);
        //получение строки уже по известной длине
        rc = recv(sock, buffer.get(), line_len, 0);
        if (rc == -1)
            throw std::system_error(errno, std::generic_category(), "Filtered log line recv error");
        std::string line(buffer.get(), rc);
        filtered_logs.push_back(line);
    }
    return filtered_logs;

}
int main()
{
    std::unique_ptr<char> buffer(new char[1024]);
    char buff[1024];
    Weak::MD5 hash;
    std::unique_ptr <sockaddr_in> self_addr(new sockaddr_in);
    std::unique_ptr <sockaddr_in> serv_addr(new sockaddr_in);
    self_addr->sin_family = AF_INET;
    self_addr->sin_port = 0;
    self_addr->sin_addr.s_addr = 0;
    serv_addr->sin_family = AF_INET;
    serv_addr->sin_port = htons(33333);
    serv_addr->sin_addr.s_addr = inet_addr("127.0.0.1");
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if ( s == -1 )
        throw std::system_error(errno, std::generic_category());
    std::cout<<"log:socket success\n";
    rc = bind(s, (sockaddr*) self_addr.get(), sizeof(sockaddr_in));
    if ( rc == -1 )
        throw std::system_error(errno, std::generic_category());
    std::cout<<"log:bind success\n";
    rc = connect(s, (sockaddr*) serv_addr.get(), sizeof(sockaddr_in));
    if ( rc == -1 )
        throw std::system_error(errno, std::generic_category());
    std::cout<<"log:connect success\n";
    //Ожидание приветственного сообщения
    int rc = recv(s,buffer.get(),1024,0);
    std::string priv_msg(buffer.get(),rc);
    if (priv_msg.substr(0,11)=="Server busy") {
        std::cout << "Подключение невозможно, сервер занят\n";
        return 1;
    }
    std::cout <<"\n" << priv_msg <<"\n" <<"\n";

    std::cout<<"---------------Фаза аутентификации-------------\n";
    //чтение базы для ассоциации имен и id
    std::map<std::string, std::string> base_cont = base_read("base.txt");
    std::string username, id;
    std::cout << "Введите имя пользователя:";
    std::cin >> username;
    id = base_cont[username];
    rc = send(s, id.c_str(), id.size(), 0);
    if ( rc == -1 )
        throw std::system_error(errno, std::generic_category());
    std::cout<<"log:send id:" << id << std::endl;
    rc = recv(s, buff, sizeof buff, 0);
    if ( rc == -1 )
        throw std::system_error(errno, std::generic_category());
    buff[rc] = 0;
    std::string salt(buff, rc);
    std::cout << "receive " << rc << " bytes as SALT: " << salt <<std::endl;
    if (rc != 16) {
        std::cout << "log:Это не соль. Останов.\n";
        close(s);
        exit(1);
    }
    std::cout << "Введите пароль:";

    // Отключение отображение ввода
    termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    termios newt = oldt;
    newt.c_lflag &= ~ECHO;  // Отключение эхо
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    // Считываем пароль
    std::string password;
    std::cin >> password;

    // Восстанавливаем настройки терминала
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    std::cout << std::endl;  // Переход на новую строку после ввода


    std::string message;
    //Хэширование пароля и передача
    StringSource(salt + std::string(password.c_str()), true,
                 new HashFilter(hash, new HexEncoder(new StringSink(message))));
    rc = send(s, message.c_str(), message.length(), 0);
    if ( rc == -1 )
        throw std::system_error(errno, std::generic_category());
    std::cout << "log:send message: "<<message<<std::endl;
    rc = recv(s, buff, 2, 0);
    if ( rc == -1 )
        throw std::system_error(errno, std::generic_category());
    buff[rc] = 0;
    std::cout << "log:receive " << rc << " bytes as answer: " << buff << std::endl;
    if (buff[0] != 'O' && buff[1] != 'K') {
        std::cout << "Не \"OK\" после аутентификации. Останов.\n";
        close(s);
        exit(1);
    }
    //Ожидание сообщения о статусе
    bool status=0;
    rc = recv(s,buffer.get(),1024,0);
    std::string status_msg(buffer.get(),rc);
    if (status_msg=="1") {
        status=1;
        std::cout <<"Статус: администратор\n";
    } else
        std::cout <<"Статус: обычный пользователь\n";

    std::cout << "----------------Отправка файлов----------------" << std::endl;
    std::cout << "--------------(exit - для выхода)--------------" << std::endl;
    if (status)
        std::cout << "---(show - просмотр лог-файла, delete - удаление лог-файла)---" << std::endl;
    std::string f_name;
    while(1) {
        //отправка имени файла
        std::cout << "Введите имя файла:";
        std::cin >> f_name;
        //проверка прав
        if((status != 1 && f_name == "show") || (status != 1 && f_name == "delete")){
            std::cout << "Недостаточно прав"<< std::endl;
            continue;
        }
        send_name(s, f_name);
        //send(s, f_name.c_str(), f_name.length(), 0);
        if (f_name == "exit") {
            close(s);
            std::cout << "Сеанс завершен"<< std::endl;
            break;
        }
        if (f_name == "show") {
            //получение информации о дате
            std::string date, month, year;
            std::cout<< "введите число: ";
            std::cin >> date;
            std::cout<< "месяц(3-первые буквы месяца на английском с заглавной): ";
            std::cin >> month;
            std::cout<< "Год(4 цифры): ";
            std::cin >> year;
            std::string show_msg=date+":"+month+":"+year;
            //отправка строки с датой
            send(s, show_msg.c_str(), show_msg.size(), 0);
            //получение ответа
            std::unique_ptr<char> serv_msg(new char[1024]);
            rc = recv(s,serv_msg.get(),2,0);
            std::string service_msg(serv_msg.get(),rc);
            if(service_msg!="OK") {
                std::cout << "Некорректная дата" << std::endl;
                continue;
            }
            std::vector<std::string> log = get_log(s);
            int index=0;
            std::cout << "----------------Журнал на "<< show_msg << "----------------" << std::endl;
            for (const auto log_line : log) {
                index++;
                std::cout<<index<<'\t'<<log_line<< std::endl;
            }
            std::string decision;
            std::cout << "Хотите редактировать лог на данное число?(y/n) ";
            std::cin >> decision;
            if(decision.length()!=1) {
                std::cout << "Некорректный ответ, необходимо(y/n)";
                continue;
            }
            if(decision == "y") {
                send(s, decision.c_str(), decision.size(), 0);
                //получение сервисного сообщения
                rc = recv(s,buffer.get(),2,0);
                if ( rc == -1 )
                    throw std::system_error(errno, std::generic_category());
                std::string service_msg(buffer.get(),rc);
                if(service_msg!="OK") {
                    continue;
                }
                size_t number;
                std::string edited_str;
                //прием номера строки для редактирования и ее отправка на сервер
                std::cout << "Номер строки для редактирования: ";
                std::cin >> number;
                send(s, &number, sizeof(number), 0);
                if ( rc == -1 )
                    throw std::system_error(errno, std::generic_category());

                //прием строки для редактирования, отправка ее длины, п потом ее самой на сервер
                std::cout << "Измененная строка: ";
                std::cin >> edited_str;
                size_t edited_str_size = edited_str.size();
                //длина строки
                rc = send(s, &edited_str_size, sizeof(edited_str_size), 0);
                if ( rc == -1 )
                    throw std::system_error(errno, std::generic_category());
                //измененная строка
                rc = send(s, edited_str.c_str(), edited_str.size(), 0);
                if ( rc == -1 )
                    throw std::system_error(errno, std::generic_category());
                //получение сервисного сообщения
                rc = recv(s,buffer.get(),2,0);
                if ( rc == -1 )
                    throw std::system_error(errno, std::generic_category());
                std::string ok_msg(buffer.get(),rc);
                if(ok_msg=="OK") {
                    std::cout << "Log файл был изменен успешно"<< std::endl;
                }
                continue;
            }
            if(decision == "n"){
                send(s, decision.c_str(), decision.size(), 0);
                //получение сервисного сообщения
                rc = recv(s,buffer.get(),2,0);
                if ( rc == -1 )
                    throw std::system_error(errno, std::generic_category());
                std::string service_msg(buffer.get(),rc);
                continue;
            }
        }
        if (f_name == "delete") {
            rc = recv(s,buffer.get(),2,0);
            if ( rc == -1 )
                throw std::system_error(errno, std::generic_category());
            std::string service_msg(buffer.get(),rc);
            if(service_msg=="OK") {
                std::cout << "Log файл был удален"<< std::endl;
            }
            continue;
        }
        // if (f_name == "exit");
        //Прием служебного сообщения
        rc = recv(s,buffer.get(),1024,0);
        std::string serv_msg(buffer.get(),rc);
        std::cout << serv_msg << std::endl;
        if(serv_msg!="OK") {
            continue;
        }
        //отправка файла
        send_file(s, f_name);
    }
}
