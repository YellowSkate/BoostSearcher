#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <mutex>
#include <unordered_map>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <jsoncpp/json/json.h>

#include "cppjieba/Jieba.hpp"

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::string;
using std::unordered_map;
using std::vector;
namespace ns_util
{
    class FileUtil
    {
    public:
        static bool ReadFile(const string &file_path, string *out)
        {
            std::ifstream in(file_path, std::ios::in);
            if (!in.is_open())
            {
                cerr << "open is fail :" << file_path << endl;
                return false;
            }

            string line;
            while (std::getline(in, line))
            { // getline()第一个参数 是一个文件输入流,返回值重载了强制类型转化,第三个参数是分隔符,
                *out += line;
            }
            in.close();
            return true;
        }
    };

    class StringUtil
    {
    public:
        static void CutString(const string &target, vector<string> *results, const string &sep)
        {
            boost::split(*results, target, boost::is_any_of(sep), boost::token_compress_on); // token_compress默认为off..
        }
    };
    const char *const DICT_PATH = "./dict/jieba.dict.utf8";
    const char *const HMM_PATH = "./dict/hmm_model.utf8";
    const char *const USER_DICT_PATH = "./dict/user.dict.utf8";
    const char *const IDF_PATH = "./dict/idf.utf8";
    const char *const STOP_WORD_PATH = "./dict/stop_words.utf8";
    class JiebaUtil
    {
    private:
        static cppjieba::Jieba jieba;

    public:
        static void Cut_Search(const string &src, vector<string> *out)
        {
            jieba.CutForSearch(src, *out);
        }
    };
    cppjieba::Jieba JiebaUtil::jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH); // 静态成员初始化

}

namespace ns_index
{
    struct F_elem // 正排元素
    {             // DocInfo
        string title;
        string content;
        string url;
        uint64_t doc_id;
    };
    struct I_elem // 倒排元素
    {
        uint64_t doc_id;
        string word;
        int weight; // 权重
    };

    typedef vector<I_elem> InvertedList; // 倒排拉链

    class Index // 单例
    {

    public:
        ~Index() {}

    private:
        Index() {}
        Index(const Index &) = delete;
        Index &operator=(const Index &) = delete;

    private:
        vector<F_elem> forward_index;                       // 正排索引表
        unordered_map<string, InvertedList> inverted_index; // 倒排索引表(哈希)

        static Index *instance;
        static std::mutex mtx;

    public:
        // 获取单例
        static Index *GetInstance()
        {
            if (nullptr == instance)
            {
                mtx.lock();
                if (nullptr == instance)
                {
                    instance = new Index();
                }
                mtx.unlock();
            }
            return instance;
        }

    private:
        F_elem *Build_Findex(const string &line)
        { // 建立正排索引
            // 切割字符串
            vector<string> results;
            const string SEP = "\3";

            ns_util::StringUtil::CutString(line, &results, SEP);
            // 填充F_elem信息
            if (results.size() != 3)
                return nullptr;

            F_elem doc;
            doc.title = results[0];
            doc.content = results[1];
            doc.url = results[2];

            doc.doc_id = forward_index.size(); // 先保存id,后插入
            forward_index.push_back(std::move(doc));

            return &forward_index.back(); // 返回最后一个Doc
        }
        bool Build_Iindex(const F_elem &doc)
        {
            // 词频统计结构
            struct word_cnt
            {
                int title_cnt;
                int content_cnt;

                word_cnt() : title_cnt(0), content_cnt(0) {}
            };

            unordered_map<string, word_cnt> word_map; // 存储词频的映射表

            // jieba分词
            vector<string> title_words;
            vector<string> content_words;
            ns_util::JiebaUtil::Cut_Search(doc.title, &title_words);
            ns_util::JiebaUtil::Cut_Search(doc.content, &content_words);
            // 统计词频
            for (auto s : title_words)
            {
                boost::to_lower(s); // 转换成小写
                word_map[s].title_cnt++;
            }
            for (auto s : content_words)
            {
                boost::to_lower(s); // 转换成小写
                word_map[s].content_cnt++;
            }
            // 插入倒排拉链
            // int i=1;
            for (const auto &word_pair : word_map)
            {
                // 填充
                I_elem item;
                item.doc_id = doc.doc_id;
                item.word = word_pair.first;

#define X (10)
#define Y (1)
                item.weight = X * word_pair.second.title_cnt + Y * word_pair.second.content_cnt; // 索引权重姑且用 10*title_cnt +conten_cnt
                // {
                //     i++;
                //     if(10>i){
                //         cout<<"["<<item.weight<<" = X*"<<word_pair.second.title_cnt <<"+ Y*"<<word_pair.second.content_cnt<<"]"<<endl;
                //     }
                // }
                // 插入
                // inverted_index[word_pair.first].push_back[item];
                InvertedList &Inverted_List = inverted_index[word_pair.first];
                Inverted_List.push_back(std::move(item));
            }
            return true;
        }

    public:
        F_elem *GetFoward_elem(uint64_t doc_id)
        { // 文档id --> 文档内容
            if (doc_id >= forward_index.size())
            {
                cerr << "Error::doc_id out range!" << endl;
                return nullptr;
            }
            return &forward_index[doc_id];
        }
        InvertedList *GetInvertedInvertedList(const string &word)
        { // 根据关键字  --> 倒排拉链
            unordered_map<string, InvertedList>::iterator iter = inverted_index.find(word);
            if (iter == inverted_index.end())
            {
                cerr << "Error::have no InvertedList!" << endl;
                return nullptr;
            }
            return &(iter->second);
        }

        bool BuildIndex(const string &input)
        { // 根据raw_html文档 建立正排和倒排索引
            std::ifstream in(input, std::ios::in | std::ios::binary);
            if (!in.is_open())
            {
                cerr << "[Error] BuildIndex() open fail :" << input << endl;
                return false;
            }
            string line;
            // int count = 0;
            while (getline(in, line))
            {

                F_elem *doc = Build_Findex(line); // 建立正排索引的同时,返回新建立的索引给后面建立倒排索引
                if (nullptr == doc)
                {
                    cerr << "[Fail] Build_Findex fail: " << line << endl;
                    continue;
                }

                // cout << "DEBUG" << doc->doc_id << " | " << doc->url << endl;
                if (!Build_Iindex(*doc))
                {
                    cerr << "[Fail] Build_Iindex fail: " << line << endl;
                    return false;
                }
                // { // debug 打印输出
                //     count++;
                //     // if (0 == count % 50)
                //     cout << "complet for :" << count <<"[url: "<<doc->url<<"]"<< endl;
                // }
            }
            return true;
        }

    public: // debug
        void Show_Fidex()
        {
            cout << "===============正排索引表=============" << endl;
            for (const auto &a : forward_index)
            {
                cout << a.doc_id << " | " << a.url << endl;
            }
            cout << "===============正排索引表=============" << endl;
        }
        void Show_Iidex_elem()
        {
            cout << "+++++++++++++++倒排索引表+++++++++++++++" << endl;
            for (const auto &a : inverted_index)
            {
                int cnt = 0;
                cout << a.first << " | ";
                for (const auto &id : a.second)
                {
                    cnt++;
                    cout << "[" << id.doc_id << ":" << id.word << ":" << id.weight << "]";
                    if (cnt % 5 == 0)
                        break;
                }
                cout << endl;
            }
            cout << "+++++++++++++++倒排索引表+++++++++++++++" << endl;
        }
    };
    Index *Index::instance = nullptr;
    std::mutex Index::mtx;
}

namespace ns_serach
{
    const string SEP="\4";//搜索关键字的分隔符
    class Searcher
    {
    private:
        ns_index::Index *index;

    public:
        Searcher() {}
        ~Searcher() {}

    public:
        void Init_Searcher(const string& input)
        {
            //获取index对象
            index= ns_index::Index::GetInstance();
            cout << "GetInstance success: addr->" << index<< endl;
            //建立索引
            index->BuildIndex(input);
            cout<<"BuildIndex Success"<<endl;
        }
        void Search(const string &query,string *json_string){
            //1.对搜索请求进行分词
            vector<string> words;
            ns_util::JiebaUtil::Cut_Search(query,&words);
            //2.进行索引
            ns_index::InvertedList I_index_list_all;
            unordered_map<uint64_t,ns_index::I_elem> token_map;
            for(string word:words){
              //转换成小写的进行索引
              boost::to_lower(word);
              ns_index::InvertedList *I_list=index->GetInvertedInvertedList(word);
              if(nullptr == I_list) continue;
            //I_index_list_all.insert(I_index_list_all.end(),I_list->begin(),I_list->end());//没有去重
              for(const auto& elem: *I_list){//通过 unordered_mao去重
                auto &item=token_map[elem.doc_id];
                item.doc_id =elem.doc_id;
                item.weight+=elem.weight;
                item.word+=elem.word;
                item.word+=SEP;//以'\4'作为搜索关键字切吃的分割符;
                
              }

            }
            //合并索引结果
            for(const auto &item : token_map){
                    I_index_list_all.push_back(std::move(item.second));
            }
            //3.汇总查找结果,按照权重排序
            std::sort(I_index_list_all.begin(),I_index_list_all.end(),\
                        [](const ns_index::I_elem &e1,const ns_index::I_elem &e2){
                            return e1.weight>e2.weight;
                        });
            //4.构建查找结果
            Json::Value root;
            for(auto& item: I_index_list_all){
                ns_index::F_elem* doc=index->GetFoward_elem(item.doc_id);
                if(nullptr == doc) continue;
                
                Json::Value elem;
                elem["title"]=doc->title;
                string key=GetKey(item.word);//得到第一个搜搜关键字
                elem["desc"]=GetDesc(doc->content,key);
                elem["url"]=doc->url;
                {//debug
                elem["weight"]=item.weight;
                elem["key"]=item.word;
                }
                root.append(elem);
            }
            Json::StyledWriter writer;
            *json_string =writer.write(root);
        }
    private:
        string GetKey(const string& s){
            // cout<<"s:"<<s<<endl;
            string key{};
            for(const auto& ch:s){
                if(ch == SEP[0]) break;
                key+=ch;
            }
            // cout<<"key:"<<key<<endl;
          
            return key;
        }
        string GetDesc(const string &html_content,const string &word){
            const int prev_step=50;
            const int next_step=100;

            auto iter=std::search(html_content.begin(),html_content.end(),word.begin(),word.end(),[](int x,int y){
                return (std::tolower(x)==std::tolower(y));
            });
            if(iter == html_content.end()) return "None1";

            int pos=std::distance(html_content.begin(),iter);

            int start=0;
            int end=html_content.size();

            if(pos-prev_step>start) start=pos-prev_step;
            if(pos+next_step<end) end=pos+next_step;

            //截取
            if(start >= end) return "Non2";
            return html_content.substr(start,end-start);
        } 
    };

}
