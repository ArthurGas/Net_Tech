#include "server_error.h"
#include <string>
#include <fstream>
#include <ctime>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>
std::string Error::error_log(std::string e_str)
{
    std::string log_str;
    std::ofstream file(log_file_path.c_str(), std::ios::app);
    if (file.is_open()) {
        std::time_t now = time(0);
        std::string date=std::ctime(&now);
        std::replace(date.begin(), date.end(), ' ', '-');
        log_str=":"+e_str;
        date.erase(std::remove(date.begin(), date.end(), '\n'), date.cend());
        if(iscritical)
            file <<date+log_str+" - Critical error\n";
        else
            file <<date+log_str+" - Not Critical error\n";
        file.close();
    } else {
        throw log_error("Open log file error");
    }
    return log_str;
}
std::string Error::regular_log(std::string reg_str, std::string id_cl, int bytes)
{
    std::string log_str;
    std::ofstream file(log_file_path.c_str(), std::ios::app);
    if (file.is_open()) {
        std::time_t now = time(0);
        std::string date=std::ctime(&now);
        std::replace(date.begin(), date.end(), ' ', '-');//замена в дате пробелов на "-"
        if(id_cl=="N/A")
            log_str=":"+reg_str;//если имя пользователя не указано, лог записывается без него
        else
            log_str=":"+id_cl+":"+reg_str;
        if(bytes>0)
            log_str=log_str+":"+"Получено байт "+ std::to_string(bytes);
        date.erase(std::remove(date.begin(), date.end(), '\n'), date.cend());
        file << date+log_str+"\n";
        //std::cout<< "\t" << "2" << "\t"<< log_str <<"\n";
        //std::cout << log_file_path << "\n";
        file.flush();
        file.close();
    } else {
        throw log_error("Open log file error");
    }
    return log_str;
}
/*
std::string Error::regular_log(std::string reg_str, std::string id_cl, int bytes)
{
    std::string log_str;
    std::ofstream file(log_file_path.c_str(), std::ios::app);
    if (file.is_open()) {
        std::time_t now = time(0);
        std::tm* ltm = std::localtime(&now);

        std::ostringstream date_stream;
        date_stream << std::put_time(ltm, "%Y-%m-%d %H:%M:%S");

        if (id_cl == "N/A")
            log_str = "|N/A|" + reg_str;
        else
            log_str = "|" + id_cl + "|" + reg_str;

        if (bytes > 0)
            log_str += "|BYTES=" + std::to_string(bytes);

        file << "LOG:" << date_stream.str() << log_str << "\n";
        file.flush();
        file.close();
    } else {
        throw log_error("Open log file error");
    }
    return log_str;
}
*/
void Error::set_path(const std::string& lg_name)
{
    if (path_locked) {
        std::cerr << "[warning] Попытка повторной установки пути к лог-файлу проигнорирована!\n";
        return;
    }
    log_file_path = lg_name;
    path_locked = true;
}
void Error::critic_set()
{
    iscritical=true;
}
