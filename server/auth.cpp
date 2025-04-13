#include "auth.h"
#include "server_error.h"

namespace cpp = CryptoPP;

void Auth::set_base(std::map<std::string, User> base_c)
{
    base_cont=base_c;
}
void Auth::operator()(int sock, std::shared_ptr<Error> er)
{
    work_sock = sock;
    Auth::auth(er);
}
//модуль заполнения базы пользователей с помощью словаря
std::map<std::string, User> Auth::base_read(std::string file_name)
{

    std::string sep=":";
    size_t pos_1, pos_2;
    std::string buf, login, pass, status_str;
    std::map<std::string, User> base;

    std::ifstream file(file_name.c_str());
    if (file.is_open()) {
        if(file.peek() == std::ifstream::traits_type::eof())
            throw std::length_error("base File Is Empty");
        while(getline(file, buf)) {
            while (buf.find(" ") < buf.size()) {
                buf.erase(buf.find(" "), 1);
            }
            pos_1 = buf.find(sep);
            pos_2 = buf.find(sep, pos_1+1);
            login = buf.substr(0, pos_1);
            pass = buf.substr(pos_1+1, pos_2-pos_1-1);
            status_str = buf.substr(pos_2+1, 1);
            bool status = (status_str == "1");
            base[login] = User{pass, status};
        }
        file.close();
        return base;
    } else {
        throw std::system_error(errno, std::generic_category(), "base Read Error");
    }
}
std::string Auth::string_recv(std::shared_ptr<Error> er)
{
    int rc;
    int buflen = 16;
    std::unique_ptr <char[]> buf(new char[buflen]);

    while (true) {
        rc = recv(work_sock, buf.get(), buflen, MSG_PEEK);
        if (rc == -1) {
            er->critic_set();
            throw std::system_error(errno, std::generic_category(), "Recv string error");
        }
        if (rc < buflen)
            break;
        buflen *= 2;
        buf = std::unique_ptr<char[]>(new char[buflen]);
    }
    std::string res(buf.get(), rc);
    rc = recv(work_sock, nullptr, rc, MSG_TRUNC);
    if (rc == -1) {
        er->critic_set();
        throw std::system_error(errno, std::generic_category(), "Clear bufer error");
    }
    res.resize(res.find_last_not_of("\n\r") + 1);
    return res;
}
bool Auth::id_check(std::string id, std::map<std::string, User> base_c)
{
    std::map<std::string, User> base(base_c);
    if (base.find(id) == base.end()) {
        throw auth_error("Auth error: Identification error");
    }
    return true;
}
std::string Auth::salt_gen()
{
    std::mt19937_64 gen(time(nullptr));
    uint64_t rnd = gen();
    std::string salt;

    cpp::StringSource((const cpp::byte*)&rnd,
                      8,
                      true,
                      new cpp::HexEncoder(new cpp::StringSink(salt)));
    return(salt);
}
std::string Auth::pw_hash(std::string salt, std::string password)
{
    std::string hashed_pw;

    cpp::Weak::MD5 hash;
    cpp::StringSource(salt + std::string(password),
                      true,
                      new cpp::HashFilter(hash, new cpp::HexEncoder(new cpp::StringSink(hashed_pw))));
    return hashed_pw;
}
bool Auth::pw_check(std::string pw_from_cl, std::string hashed_pw)
{
    if (pw_from_cl != hashed_pw)
        throw auth_error("Auth error: password dismatch");
    return true;
}
void Auth::auth(std::shared_ptr<Error> er)
{
    int rc;
    std::string salt, hashed_pw;
    std::unique_ptr<char> buffer(new char[1024]);
    //получение имени пользователя
    rc = recv(work_sock,buffer.get(), 1024, 0);
    std::string id(buffer.get(),rc);
    client_id=id;
    er->regular_log("Клиент передал id", "N/A", rc);//логирование
    //std::string id = string_recv(er);
    //проверка имени пользователя
    id_check(id, base_cont);
    std::clog << "log: Пользователь: " << id << '\n';
    er->regular_log("Id обнаружен в базе", id, 0);//логирование
    
    salt=salt_gen();
    rc = send(work_sock, salt.c_str(), 16, 0);
    if (rc == -1)
        throw std::system_error(errno, std::generic_category(), "Salt send error");
    std::clog << "log: Отправка SALT " << salt << std::endl;
    hashed_pw=pw_hash(salt, base_cont[id].password);
    std::clog << "log: Ожидание хешированного пароля:" << hashed_pw << std::endl;
    
    //получение хешированного пароля
    rc = recv(work_sock,buffer.get(), 1024, 0);
    std::string pw(buffer.get(),rc);
    er->regular_log("Клиент отправил хешированный пароль", id, rc);//логирование
    
    //if (string_recv(er) != hashed_pw)
    if (pw != hashed_pw)
        throw auth_error("Auth error: password dismatch");
    std::clog <<"log: Успешный вход, отправка сообщения ""OK""\n";
    er->regular_log("Успешный вход", id, 0);//логирование
    
    rc = send(work_sock, "OK", 2, 0);
    if (rc == -1)
        throw std::system_error(errno, std::generic_category(), "Send ""OK"" error");
    //Уведомление пользователя о статусе
    status = base_cont[id].is_admin;
    std::string status_msg = std::to_string(status);
    rc = send(work_sock, status_msg.c_str(), status_msg.size(), 0);
    if (rc == -1)
        throw std::system_error(errno, std::generic_category(), "Status send error");
}
