#include "interface.h"

struct {
    uint32_t p=33333;
    std::string b="base.txt";
    std::string l="log.txt";
    uint32_t v=1;
    bool p_not_set()
    {
        return(p==33333);
    }
    bool b_not_set()
    {
        return(b=="base.txt");
    }
    bool l_not_set()
    {
        return(l=="log.txt");
    }
    bool v_not_set()
    {
        return(v==1);
    }
} params;

bool Interface::set_options(int argscount, char *argvectors[], std::shared_ptr<Error> e)
{
    po::options_description desc{"Options"};
    desc.add_options()
    ("help,h", "Show help")
    ("port,p", po::value<uint32_t>(&params.p),
     "Set server port")
    ("base,b", po::value<std::string>(&params.b),
     "Set base filename")
    ("log,l", po::value<std::string>(&params.l),
     "Set log filename")
    ("version,v", po::value<uint32_t>(&params.v),
     "Set server version(1-receive only .txt files, 2-receive only .bin files)")
    ;
// переменная для результатов парсинга
    po::variables_map vm;
// парсинг
    po::store(po::parse_command_line(argscount, argvectors, desc), vm);
// присвоение значений по умолчанию
    po::notify(vm);
    std::cout << '\t' <<"Сервер приема файлов" << '\n';
    //выполнение варианта "Выдать справку"
    if (vm.count("help")) {
        std::cout << desc << "\n";
        return false;
    }
// выполнение варианта "Выдать справку"(запуск без параметров)
    if(params.p_not_set() && params.b_not_set() && params.l_not_set() && params.v_not_set()) {
        std::cout << desc << "\n";
        std::cout << "Program use default parameters" << "\n" << "Port: " << params.p << "\n";
        std::cout << "Base filename: " << params.b<< "\n";
        std::cout << "Log filename: " << params.l<< "\n";
        std::cout << "Server version: " << params.v<< "\n";

// запуск с полностью или частично указанными параметрами
    } else {
        if(params.p_not_set())
            std::cout << "Port(default): " << params.p << "\n";
        else
            std::cout << "Port: " << params.p << "\n";
        if(params.b_not_set())
            std::cout << "Base filename(default): " << params.b<< "\n";
        else
            std::cout << "Base filename: " << params.b<< "\n";
        if(params.l_not_set())
            std::cout << "Log filename(default): " << params.l<< "\n";
        else
            std::cout << "Log filename: " << params.l<< "\n";
        if(params.v_not_set())
            std::cout << "Server version(default): " << params.v<< "\n";
        else
            std::cout << "Server version: " << params.v<< "\n";

    }
    port=params.p;
    if(port<1024 || port>65536) {
        e->critic_set();
        throw option_error("Bad port");
    }
    base_file=params.b;
    log_file=params.l;
    version=params.v;
    if(version!=1 and version!=2) {
        e->critic_set();
        throw option_error("Bad version(1 or 2)");
    }
    return true;
}

std::string Interface::get_basefile() const
{
    return base_file;
}
uint32_t Interface::get_port() const
{
    return port;
}
std::string Interface::get_logfile() const
{
    return log_file;
}
uint32_t Interface::get_version() const
{
    return version;
}