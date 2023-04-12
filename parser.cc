#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <boost/filesystem.hpp>

#include "tool.hpp"

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::string;
using std::vector;

const string src_path = "data/input/boost_1_81_0";
const string output = "data/raw_html/raw.txt";

typedef struct DocInfo
{
    string title;
    string content;
    string url;
} DocInfo_t;

static void ShowDoc(const struct DocInfo &doc, bool Content = false)
{
    cout << "title: " << doc.title << endl;
    if (true == Content)
        cout << "content: " << doc.content << endl;
    cout << "url: " << doc.url << endl;
    cout << "==================================\n\n";
}

/*补充:
    const &  输入型参数
        *    输出型参数
        &    输入输出型参数
*/
bool EnumFile(const string &src_path, vector<string> *file_list)
{
    namespace fs = boost::filesystem;
    fs::path root_path(src_path);

    if (!fs::exists(root_path))
    { // 判断路径是否存在
        cerr << "path not exists" << endl;
        return false;
    }

    fs::recursive_directory_iterator end; // 定义一个空的迭代器,用来判断迭代器是否已经到达末尾
    // boost::filesystem::recursive_directory_iterator();
    for (fs::recursive_directory_iterator iter(root_path); iter != end; iter++)
    {
        if (!fs::is_regular_file(*iter))
        { // 判断是不是常规文件,html是常规文件
          // 常规文件是指一般文件类型，它不是目录，也不是符号链接，也不是其他特殊类型的文件。
            continue;
        }
        if (iter->path().extension() != ".html")
        {
            // 判断后缀是不是.html
            continue;
        }

        // cout << "debug:: " << iter->path().string() << std::endl;
        // 输出结果 ::debug:: data/input/boost_1_81_0/doc/html/boost/type_erasure/dynamic_binding.html
        file_list->push_back(iter->path().string()); // 将文件的完整路径 保存到 list中
    }

    return true;
}

static bool ParseTitle(const string &file, string *title)
{ // static 用于保护函数
    std::size_t begin = file.find("<title>");
    std::size_t end = file.find("</title>");
    if (string::npos == begin || string::npos == end)
    {
        return false;
    }
    begin += string("<title>").size();
    if (begin > end)
    {
        return false;
    }
    *title = file.substr(begin, end - begin);
    return true;
}

static bool ParseContent(const string &file, string *content)
{
    // 去标签,一个简易的自动机
    enum stat
    {
        LABLE,
        CONTENT
    };
    enum stat s = LABLE;
    for (char c : file)
    {
        switch (s)
        {
        case LABLE:
            if (c == '>')
                s = CONTENT;
            break;
        case CONTENT:
        {
            if (c == '<')
                s = LABLE;
            else
            {
                if (c == '\n')
                    c = ' '; // 不保留 原始文件中的\n  .  \n作html解析后的文本分隔符
                content->push_back(c);
            }
        }
        break;
        default:
            break;
        }
    }
    return true;
}

static bool ParseUrl(const string &path, string *url)
{
    string url_head = "https://www.boost.org/doc/libs/1_81_0";
    string url_tatil = path.substr(src_path.size()); // src data/input/boost_1_81_0
                                                     // debug::[ data/input/boost_1_81_0/]doc/html/boost/type_erasure/dynamic_binding.html
    // 我们只需要后半部分
    *url = url_head + url_tatil;
    return true;
}

bool ParseHtml(const vector<string> &file_list, vector<DocInfo_t> *results)
{
    for (const string &file : file_list)
    {
        string result;

        if (!ns_util::FileUtil::ReadFile(file, &result))
        { // 读取逐个文件内容
            continue;
        }

        DocInfo_t doc;

        if (!ParseTitle(result, &doc.title))
        { // 提取titlt;
            continue;
        }

        if (!ParseContent(result, &doc.content))
        { // 提取content
            continue;
        }

        if (!ParseUrl(file, &doc.url))
        { // 提取url
            continue;
        }

        // ShowDoc(doc);
        //        if(doc.url=="https://www.boost.org/doc/libs/1_81_0/libs/iterator/doc/function_input_iterator.html")ShowDoc(doc,true);
        // 当 <title></title> 标签中没有内容时，Web 页面上的标题通常是默认的文件名或 URL。
        results->push_back(std::move(doc)); // move() 减少拷贝
    }

    return true;
}

bool SaveHtml(const vector<DocInfo_t> results, const string &output)
{
#define SEP '\3' // 作分隔符
    std::ofstream raw_txt(output, std::ios::out | std::ios::binary);

    if (!raw_txt.is_open())
    {
        cerr << "open fail::" << output << endl;
        return false;
    }

    // 开始写入
    for (auto &item : results)
    {
        string out;
        out += item.title;
        out += SEP;
        out += item.content;
        out += SEP;
        out += item.url;
        out += '\n';

        raw_txt.write(out.c_str(), out.size());
    }
    raw_txt.close();
    return true;
}
int main()
{
    cout << "=======parser start=========\n";
    vector<string> file_list; // 用于存放文件名(包含路径)
    if (!EnumFile(src_path, &file_list))
    {
        return 1;
    }

    vector<DocInfo_t> results; // 解析文件,对file_list(string)--> DocInfo
    if (!ParseHtml(file_list, &results))
    {
        return 2;
    }

    if (!SaveHtml(results, output))
    {
        cerr << "SaveHtml error" << endl;
        return 3;
    }
    cout << "=======parser complet=========\n";
// {//debug
//     ns_index::Index *index_table=ns_index::Index::GetInstance();
//     index_table->BuildIndex(output);

//     cout << "========buildIndex complet===============\n";
//     index_table->Show_Iidex_elem();
// }
    return 0;
}
