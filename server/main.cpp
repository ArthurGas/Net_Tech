#include <string>
#include <cstring>
#include <boost/program_options.hpp>

#include "calc.h"
#include "auth.h"
#include "interface.h"
#include "connection.h"
#include "server_error.h"

int main(int argc, char *argv[])
{
    Interface i;// объект класса интерфейса
    //Error er; 
    auto er = std::make_shared<Error>();//объект класс для обработки ошибок

    try {
        if(!i.set_options(argc, argv, er))
            return 0;
        er->set_path(i.get_logfile());
        Connection serv(i.get_port(), i.get_version());
        Send_recv c(i.get_version(), i.get_logfile()); // объект класса вычислений
        Auth a; // объект класса аутентификации
        //заполнение базы данных по файлу и установка пути лог-файла
        std::map<std::string, User>bs=a.base_read(i.get_basefile());
        a.set_base(bs);
        serv.connect(c, a, er); // передача объекту значения сокета

    } catch(std::logic_error &e) {
        std::cerr<< e.what() << std::endl;
    } catch(option_error &e) { // ошибка опций не может быть записана в неоткрытый файл
        std::cerr<< e.what() << std::endl;
    } catch(log_error &e) { // ошибка открытия лог-файла не модет быть записана
        std::cerr<< e.what() << std::endl;
    } catch(std::exception &e) {
        std::cerr<< e.what() << std::endl;
        er->error_log(e.what());
    }
    return 0 ;
}
