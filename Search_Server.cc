#include <iostream>
#include <string>
#include <vector>

#include "tool.hpp"
#include "cpp-httplib/httplib.h"


using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::string;
using std::vector;

const string input = "data/raw_html/raw.txt";
const std::string root_path = "./wwwroot";

int main(){
    ns_serach::Searcher search;
    search.Init_Searcher(input);
    
    httplib::Server svr;
    svr.set_base_dir(root_path.c_str());
        svr.Get("/s", [&search](const httplib::Request &req, httplib::Response &rsp){
            if(!req.has_param("word")){
                rsp.set_content("必须要有搜索关键字!", "text/plain; charset=utf-8");
                return;
            }
            std::string word = req.get_param_value("word");
            std::cout << "用户在搜索：" << word << std::endl;
            
            std::string json_string;
            search.Search(word, &json_string);
            rsp.set_content(json_string, "application/json");
            //rsp.set_content("你好,世界!", "text/plain; charset=utf-8");
            });

    cout<<"服务器启动成功"<<endl;
    svr.listen("0.0.0.0", 8081);

    return 0;
}

