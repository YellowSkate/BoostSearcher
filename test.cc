#include <iostream>
#include <string>
#include <vector>

// #include "tool.hpp"

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::string;
using std::vector;

// const string input = "data/raw_html/raw.txt";

// int main(){
//     ns_serach::Searcher s;
//     s.Init_Searcher(input);
//     std::string query;
//     std::string json_string;
//     char buffer[1024];
//     while(true){
//         std::cout << "Please Enter You Search Query# ";
//         fgets(buffer, sizeof(buffer)-1, stdin);
//         buffer[strlen(buffer)-1] = 0;
//         query = buffer;
//         s.Search(query, &json_string);
//         std::cout << json_string << std::endl;
//     }

//     return 0;
// }
const string SEP = "\4"; // 搜索关键字的分隔符

string GetKey(const string &s)
{
    cout << "s:" << s << endl;
    string key{};
    for (const auto &ch : s)
    {
        if (ch == SEP[0])
            break;
        key += ch;
    }
    cout << "key:" << key << endl;

    return key;
}
int main()
{

    string s = "123";
    s += SEP;
    s += "456";
    GetKey(s);
    return 0;
}