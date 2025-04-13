#include "snd_rcv.h"
std::mutex Send_recv::log_file_mutex; // Определение мьютекса

bool Send_recv::operator()(int sock, bool status, std::string cl_id, std::shared_ptr<Error> er)
{
    work_sock = sock;
    id_clc = cl_id;
    stat = status;
    return(Send_recv::calc(er));
}
std::string Send_recv::filename_recv(int sock, std::shared_ptr<Error> er)
{
    const size_t BUF_SIZE = 4096;
    uint32_t net_name_l=0;
    //прием длины имени файла в сетевом виде
    int rc = recv(sock, &net_name_l, sizeof(net_name_l), 0);
    if (rc == -1) {
        throw std::system_error(errno, std::generic_category(), "Filename length recv error");
    }
    if (rc == 0) {
        throw std::system_error(errno, std::generic_category(), "Client closed connetion");
    }
    er->regular_log("Клиент передал длину имени файла", id_clc, rc);//логирование
    //перевод в стандартный вид длины имени файла
    uint32_t name_l = ntohl(net_name_l);
    //ограничение на длину имени файла
    if (name_l == 0 || name_l > BUF_SIZE) {
        throw std::system_error(errno, std::generic_category(), "Filename length error(it's too long)");
    }
    //прием имени файла
    std::unique_ptr<char[]> name_buf(new char[name_l]);
    rc = recv(work_sock, name_buf.get(), name_l, 0);
    if (rc == -1) {
        throw std::system_error(errno, std::generic_category(), "Filename recv error");
    }
    std::string name(name_buf.get(), name_l);
    er->regular_log("Клиент передал имя файла", id_clc, rc);//логирование
    return name;
}
void Send_recv::file_recv(int sock, const std::string& ofile_name, std::shared_ptr<Error> er)
{
    int rc;
    std::string ofile_path = "Files/"+ofile_name;
    const size_t BUF_SIZE = 4096;
    //проверка сущестует ли файл, если да, то он удаляется
    std::ifstream test(ofile_path, std::ios::binary);
    if (test.is_open()) {
        test.close();
        if( remove(ofile_path.c_str()) != 0 ) {
            throw std::system_error(errno, std::generic_category(), "File size recv error");
        } else {
            std::clog << fmt::format("log: Файл с расположением {} будет перезаписан", ofile_path) << std::endl;
        }
    }
    //открытие файла на запись
    std::ofstream file(ofile_path, std::ios::binary);
    if (!file) {
        er->critic_set();
        throw std::system_error(errno, std::generic_category(), "Failed to open file");
    }
    //получение размера файла
    std::streamsize file_size;
    rc = recv(sock, &file_size, sizeof(file_size), 0) ;
    if (rc == -1) {
        er->critic_set();
        throw std::system_error(errno, std::generic_category(), "File size recv error");
        er->regular_log("Клиент передал размер файла", id_clc, rc);//логирование
    }
    //запись в файл блоками по 4096 байт
    char buffer[BUF_SIZE];
    std::streamsize received = 0;
    while (received < file_size) {
        ssize_t read_bytes = std::min(file_size - received, static_cast<std::streamsize>(BUF_SIZE));//сравнение оставшегося блока байт и максимального количества в блоке, чтение меньшего
        ssize_t rc_bytes = recv(sock, buffer, read_bytes, 0);
        if (rc_bytes == -1) {
            throw std::system_error(errno, std::generic_category(), "Bytes block recv error");
        }
        if (rc_bytes == 0) {
            throw std::system_error(errno, std::generic_category(), "Client interrupted session");
        }

        file.write(buffer, rc_bytes);
        received += rc_bytes;
    }
    er->regular_log("Клиент передал содержимое файла", id_clc, received);//логирование
}
//функция отправки содержимого логов по определенной дате

bool Send_recv::send_log(int sock, const std::string& lg_f, const std::string& date_st)
{
    //подготовка фильтра
    std::string date, month, year;
    size_t pos_1, pos_2;
    auto sep = ":";
    pos_1 = date_st.find(sep);
    pos_2 = date_st.find(sep, pos_1+1);
    date = date_st.substr(0, pos_1);
    month = date_st.substr(pos_1+1, 3);
    year = date_st.substr(pos_2+1, 4);
    if(atoi(date.c_str())>31 || month.length()!=3)
        return false;
    //отправка сервисного сообщения
    std:: string service_msg = "OK";
    send(sock, service_msg.c_str(), service_msg.size(), 0);
    //составление шаблона с помощью регулярных выражений
    std::regex pattern("^\\w+-" + month + "-" + date + "-\\d{2}:\\d{2}:\\d{2}-" + year + ":");
    //открытие файла для чтения
    std::ifstream log_file(lg_f);
    if (!log_file) {
        std::cerr << "Failed to open log file: " << lg_f << "\n";
        throw std::system_error(errno, std::generic_category(), "Open file error");
    }
    //вектор с отфильтрованными строками
    std::vector<std::string> filtered_logs;
    std::string line;
    //чтение логов построчно
    while (std::getline(log_file, line)) {
        // Проверка строки на содержание даты
        if (std::regex_search(line, pattern)) {
            filtered_logs.push_back(line);
        }
    }
    log_file.close();

    //вектор строк сформирован, теперь построчная отправка значений из вектора
    //отправка количества строк
    size_t log_count = filtered_logs.size();
    //std::cout << log_count << std::endl;
    int rc = send(sock, &log_count, sizeof(log_count), 0);
    if (rc == -1)
        throw std::system_error(errno, std::generic_category(), "Log lines count send error");
    // Затем отправляем каждую строку
    for (const auto& log : filtered_logs) {
        size_t log_size = log.size();
        // Сначала отправляем размер строки
        rc = send(sock, &log_size, sizeof(log_size), 0);
        if (rc == -1)
            throw std::system_error(errno, std::generic_category(), "Log line length send error");

        // Затем саму строку
        rc = send(sock, log.c_str(), log_size, 0);
        if (rc == -1)
            throw std::system_error(errno, std::generic_category(), "Log line send error");
    }
    return true;
}
//функция для поиска абсолютно номера строки(для редактирования строки в логе)
size_t Send_recv::find_line_number(const std::string& lg_f, const std::string& date_st, size_t local_line_num)
{
    std::lock_guard<std::mutex> lock(Send_recv::log_file_mutex);
    std::ifstream file(lg_f);
    std::string line;
    size_t global_counter = 0;
    size_t matched_counter = 0;
    std::string date, month, year;
    size_t pos_1, pos_2;
    auto sep = ":";
    pos_1 = date_st.find(sep);
    pos_2 = date_st.find(sep, pos_1+1);
    date = date_st.substr(0, pos_1);
    month = date_st.substr(pos_1+1, 3);
    year = date_st.substr(pos_2+1, 4);
    //составление шаблона с помощью регулярных выражений
    std::regex pattern("^\\w+-" + month + "-" + date + "-\\d{2}:\\d{2}:\\d{2}-" + year + ":");
    //чтение файла построчно
    while (std::getline(file, line)) {
        if (std::regex_search(line, pattern)) {
            matched_counter++;
            if (matched_counter == local_line_num) {
                return global_counter + 1; // +1 т.к. строки нумеруются с 1
            }
        }
        global_counter++;
    }
    throw std::runtime_error("Line not found");
}
//функция замены строки файле путем копирования с помощью временного файла
void Send_recv::replace_line_in_file(const std::string& lg_f, size_t line_number, const std::string& edited_line)
{
    std::lock_guard<std::mutex> lock(Send_recv::log_file_mutex);
    // Временный файл
    std::ofstream temp_file("temp_log.txt");
    std::ifstream original_file(lg_f);
    std::string line;
    size_t current_line = 0;
    //чтение лог файла по строкам во временный файл, если изменненной строки совпадает с текущей, записывается строка клиента
    while (std::getline(original_file, line)) {
        current_line++;
        if (current_line == line_number) {
            temp_file << edited_line << "\n";
        } else {
            temp_file << line << "\n";
        }
    }
    original_file.close();
    temp_file.close();

    // Заменяем исходного файла
    std::remove(lg_f.c_str());
    std::rename("temp_log.txt", lg_f.c_str());
}
void Send_recv::delete_lg_file(const std::string& lg_f)
{
    std::lock_guard<std::mutex> lock(Send_recv::log_file_mutex);
    std::remove(lg_f.c_str());
}

bool Send_recv::calc(std::shared_ptr<Error> er)
{
    std::string f_name;
    std::string f_type;
    std::string service_msg;
    while(1) {
        while(1) {
            //получение имени файла
            //std::unique_ptr<char> name_buf(new char[1024]);
            //int rc = recv(work_sock, name_buf.get(), 1024, 0);
            //std::string f_name(name_buf.get(), rc);
            f_name = filename_recv(work_sock, er);
            std::clog << "log: Имя файла получено успешно " << f_name << std::endl;
            if (f_name=="exit") {
                return false;
            }
            //обработка команды для просмотра логов
            if (f_name=="show") {
                std::string date, month, year;
                std::unique_ptr<char> buf(new char[1024]);
                //прием даты, введенной клиентом
                int rc = recv(work_sock, buf.get(), 1024, 0);
                if (rc == -1)
                    throw std::system_error(errno, std::generic_category(), "Log line length send error");
                std::string date_str(buf.get(), rc);
                //функция формирования списка из строк лога и отправки строк клиенту
                bool flag = send_log(work_sock, lgfile_path, date_str);
                //отправка сервисного "NO" если дата некорректна
                if (!flag) {
                    service_msg = "NO";
                    send(work_sock, service_msg.c_str(), service_msg.size(), 0);
                }
                //прием решения клиента для просмотра лога
                rc = recv(work_sock, buf.get(), 1, 0);
                std::string des_str(buf.get(), rc);
                //отправка ОК
                service_msg="OK";
                send(work_sock, service_msg.c_str(), service_msg.size(), 0);
                if (rc == -1)
                    throw std::system_error(errno, std::generic_category(), "OK send error");
                if(des_str=="y") {
                    //отправка сервисного сообщения клиенту
                    send(work_sock, "OK", 2, 0);
                    //получение номера строки по текущей дате
                    size_t number, edited_str_size;
                    rc = recv(work_sock, &number, sizeof(number), 0);
                    if (rc == -1)
                        throw std::system_error(errno, std::generic_category(), "String number recv error");
                    //получение длины измененной строки
                    rc = recv(work_sock, &edited_str_size, sizeof(edited_str_size), 0);
                    if (rc == -1)
                        throw std::system_error(errno, std::generic_category(), "Edited str size recv error");
                    //получение измененной строки
                    rc = recv(work_sock, buf.get(), edited_str_size, 0);
                    if (rc == -1)
                        throw std::system_error(errno, std::generic_category(), "Edited str recv error");
                    std::string edited_str(buf.get(), rc);
                    //получение глобального номера строки
                    size_t gl_num = find_line_number(lgfile_path, date_str, number);
                    //замена строки
                    replace_line_in_file(lgfile_path, gl_num, edited_str);
                    er->regular_log("Клиент изменил лог файл", id_clc, 0);//логирование
                    //отправка служебного сообщения
                    service_msg="OK";
                    send(work_sock, service_msg.c_str(), service_msg.size(), 0);
                }
                continue;
            }
            if(f_name=="delete") {
                std::unique_ptr<char> buf(new char[10]);
                delete_lg_file(lgfile_path);
                service_msg="OK";
                send(work_sock, service_msg.c_str(), service_msg.size(), 0);
                continue;
            }

            //проверка соответствия расширения
            f_type = f_name.substr(f_name.find('.')+1,3);
            std::cout<<f_type<<"\n";
            if(serv_v == 1 && f_type!="txt") {
                er->regular_log("Клиент ввел некорректное имя файла", id_clc, 0);//логирование
                service_msg="Invalid extension: txt only";
                send(work_sock, service_msg.c_str(), service_msg.size(), 0);
                std::cout << "log: Ожидание повторной отправки имени файла" << std::endl;
                continue;
            }
            if(serv_v == 2 && f_type!="bin") {
                er->regular_log("Клиент ввел некорректное имя файла", id_clc, 0);//логирование
                service_msg="Invalid extension: bin only";
                send(work_sock, service_msg.c_str(), service_msg.size(), 0);
                std::cout << "log: Ожидание повторной отправки имени файла" << std::endl;
                continue;
            }
            service_msg="OK";
            send(work_sock, service_msg.c_str(), service_msg.size(), 0);
            break;
        }
        //получение файла
        file_recv(work_sock, f_name, er);
        std::clog << "log: Файл был получен успешно "<< std::endl;
    }
    return true;
}
