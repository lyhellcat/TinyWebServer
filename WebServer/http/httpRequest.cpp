#include <algorithm>
#include <regex>
#include <cassert>

#include "httpRequest.h"
#include "sqlconnpool.h"
#include "log.h"

using namespace std;

const unordered_set<string> HttpRequest::DEFAULT_HTML {
    "/index", "/register", "/login", "/welcome", "/video", "/picture",
};

const unordered_map<string, int> HttpRequest::DEFAULT_HTML_TAG {
    {"/register.html", 0},
    {"/login.html", 1},
};

void HttpRequest::Init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool HttpRequest::IsKeepAlive() const {
    if (header_.count("Connection") == 1) {
        return header_.find("Connection")->second
             == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HttpRequest::parse(Buffer& buff) {
    const char CRLF[] = "\r\n";
    if (buff.ReadableBytes() <= 0) {
        return false;
    }
    while (buff.ReadableBytes() && state_ != FINISH) {
        // Find "\r\n" in request
        const char *lineEnd = search(buff.ReadPtr(), buff.ConstWritePtr(), CRLF, CRLF + 2);
        string line(buff.ReadPtr(), lineEnd);

        switch (state_) {
        case REQUEST_LINE:
            if (!ParseRequestLine_(line)) {
                return false;
            }
            ParsePath_();
            break;
        case HEADER:
            ParseHeader_(line);
            if (buff.ReadableBytes() <= 2 || method_ == "GET") {
                state_ = FINISH;
            }
            break;
        case BODY:
            ParseBody_(line);
            break;
        default:
            break;
        }
        if (lineEnd == buff.WritePtr()) {
            break;
        }
        buff.UpdateReadPtrUntilEnd(lineEnd + 2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());

    return true;
}


void HttpRequest::ParsePath_() {
    if (path_ == "/") {
        path_ = "/index.html";
    } else {
        // Find path in DEFAULT_HTML
        for (auto &item : DEFAULT_HTML) {
            if (item == path_) {
                path_ += ".html";
                break;
            }
        }
    }
}

bool HttpRequest::ParseRequestLine_(const string &line) {
    // stringstream ss(line);
    // ss >> method_ >> path_ >> version_;
    // version_ = version_.substr(5);
    regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch subMatch;
    if (regex_match(line, subMatch, patten)) {
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        state_ = HEADER;
        // method_ = "GET";
        // path_ = "/";
        // version_ = "1.1";
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}

void HttpRequest::ParseHeader_(const string &line) {
    // Header  Key: value
    regex patten("^([^ :]*): ?(.*)$");
    smatch subMatch;
    if (regex_match(line, subMatch, patten)) {
        header_[subMatch[1]] = subMatch[2];
    } else {
        state_ = BODY;
    }
}

void HttpRequest::ParseBody_(const string &line) {
    body_ = line;
    ParsePost_();
    state_ = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

int HttpRequest::ConvertHex(char ch) {
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    return ch;
}

void HttpRequest::ParsePost_() {
    if (method_ == "POST" &&
        header_["Content-Type"] == "application/x-www-form-urlencoded") {
        ParseFromURLencoded_();
        if (DEFAULT_HTML_TAG.count(path_)) {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag:%d", tag);
            if (tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                if (UserVerify(post_["username"], post_["password"], isLogin)) {
                    path_ = "/welcome.html";
                } else {
                    path_ = "/error.html";
                }
            }
        }
    }
}

void HttpRequest::ParseKeyValue_(const string &line) {
    size_t pos = line.find_first_of("=");
    // cout << line.substr(0, pos) << " " << line.substr(pos + 1) << endl;
    post_[line.substr(0, pos)] = line.substr(pos + 1);
}

// Parse "username=hellcat&password=123456" --> ["username": "hellcat", "password": "123456"]
void HttpRequest::ParseFromURLencoded_() {
    if (body_.size() == 0) {
        return;
    }
    size_t pre = 0;
    size_t found = body_.find_first_of("&");
    while (found != string::npos) {
        ParseKeyValue_(body_.substr(pre, found - pre));
        pre = found + 1;
        found = body_.find_first_of("&", found + 1);
    }
    ParseKeyValue_(body_.substr(pre, found - pre));
}

bool HttpRequest::UserVerify(const string &name, const string &pwd,
                             bool isLogin) {
    if (name == "" || pwd == "") {
        return false;
    }
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
    MYSQL *sql;
    SqlConnect(&sql, SqlConnPool::Instance());
    assert(sql);

    bool flag = false;
    unsigned int j = 0;
    char order[256] = {0};
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;

    if (!isLogin) {
        flag = true;
    }
    // Query user and password
    snprintf(order, 256,
             "SELECT username, password FROM user WHERE username='%s' LIMIT 1",
             name.c_str());
    LOG_DEBUG("%s", order);

    if (mysql_query(sql, order)) {
        mysql_free_result(res);
        return false;
    }
    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);

    while (MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        string password(row[1]);
        // Registration act and username is not used
        if (isLogin) {
            if (pwd == password) {
                flag = true;
            } else {
                flag = false;
                LOG_DEBUG("pwd error!");
            }
        } else {
            flag = false;
            LOG_DEBUG("user used!");
        }
    }
    mysql_free_result(res);

    // Registration act and username is not used
    if (!isLogin && flag == true) {
        LOG_DEBUG("regirster!");
        snprintf(order, 256,
                 "INSERT INTO user(username, password) VALUES('%s','%s')",
                 name.c_str(), pwd.c_str());
        LOG_DEBUG("%s", order);
        if (mysql_query(sql, order)) {
            LOG_DEBUG("Insert error!");
            flag = false;
        }
        flag = true;
    }
    LOG_DEBUG("UserVerify success!!");
    return flag;
}

string HttpRequest::path() const { return path_; }

string &HttpRequest::path() { return path_; }

string HttpRequest::method() const { return method_; }

string HttpRequest::version() const { return version_; }

string HttpRequest::GetPost(const string &key) const {
    assert(key != "");
    if (post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}
