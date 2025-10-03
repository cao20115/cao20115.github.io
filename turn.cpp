#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>
#include <cstdlib>

std::string markdownToHtml(const std::string& markdown) {
    std::istringstream iss(markdown);
    std::ostringstream oss;
    std::string line;

    bool inCodeBlock = false;
    std::string codeLang;
    std::ostringstream codeBuffer;
    // 脚注收集
    std::map<std::string, std::string> footnotes;
    std::vector<std::pair<std::string, std::string>> footnoteOrder;
    int lineNum = 0;
    // 列表状态
    bool inUl = false, inOl = false;
    while (std::getline(iss, line)) {
        lineNum++;
        std::smatch match;
        // 脚注定义: [^id]: 内容
        if (std::regex_match(line, match, std::regex(R"(^\s*\[\^([a-zA-Z0-9_-]+)\]:\s*(.*)$)"))) {
            footnotes[match[1]] = match[2];
            footnoteOrder.emplace_back(match[1], match[2]);
            continue;
        }
        // 代码块开始 ```lang 或 ~~~lang
        if (!inCodeBlock && (std::regex_match(line, match, std::regex(R"(^\s*(```|~~~)\s*([a-zA-Z0-9]*)\s*$)")))) {
            inCodeBlock = true;
            codeLang = match[2];
            codeBuffer.str("");
            continue;
        }
        // 代码块结束
        if (inCodeBlock && std::regex_match(line, std::regex(R"(^\s*(```|~~~)\s*$)"))) {
            oss << "<pre><code class=\"language-" << codeLang << "\">" << codeBuffer.str() << "</code></pre>\n";
            inCodeBlock = false;
            codeLang.clear();
            continue;
        }
        if (inCodeBlock) {
            codeBuffer << line << "\n";
            continue;
        }
        // 有序列表 1. ...
        if (std::regex_match(line, match, std::regex(R"(^\s*\d+\.\s+(.+)$)"))) {
            if (!inOl) { oss << "<ol>\n"; inOl = true; }
            oss << "<li>" << match[1] << "</li>\n";
            continue;
        } else if (inOl && !std::regex_match(line, std::regex(R"(^\s*\d+\.\s+(.+)$)"))) {
            oss << "</ol>\n"; inOl = false;
        }
        // 无序列表 - ... 或 * ... 或 + ...
        if (std::regex_match(line, match, std::regex(R"(^\s*([-*+])\s+(.+)$)"))) {
            if (!inUl) { oss << "<ul>\n"; inUl = true; }
            oss << "<li>" << match[2] << "</li>\n";
            continue;
        } else if (inUl && !std::regex_match(line, std::regex(R"(^\s*([-*+])\s+(.+)$)"))) {
            oss << "</ul>\n"; inUl = false;
        }
        // 引用 >
        if (std::regex_match(line, match, std::regex(R"(^\s*>\s?(.*)$)"))) {
            oss << "<blockquote>" << match[1] << "</blockquote>\n";
            continue;
        }
        // 标题
        if (std::regex_match(line, match, std::regex(R"(^\s*###\s*(.*))"))) {
            oss << "<h3>" << match[1] << "</h3><hr>\n";
        } else if (std::regex_match(line, match, std::regex(R"(^\s*##\s*(.*))"))) {
            oss << "<h2>" << match[1] << "</h2><hr>\n";
        } else if (std::regex_match(line, match, std::regex(R"(^\s*#\s*(.*))"))) {
            oss << "<h1>" << match[1] << "</h1><hr>\n";
        }
        else
        {
            // LaTeX 块公式 $$...$$
            if (std::regex_match(line, match, std::regex(R"(^\s*\$\$(.*)\$\$\s*$)"))) {
                oss << "<div class=\"math\">$$" << match[1] << "$$</div>\n";
            } else {
                // 行内公式 $...$
                std::string line2 = std::regex_replace(line, std::regex(R"(\$(.+?)\$)"), "<span class=\"math\">$${1}$$</span>");
                // 行间代码 `code`
                line2 = std::regex_replace(line2, std::regex(R"(`([^`]+)`)") , "<code>$1</code>");
                // 图片 ![alt](url)
                line2 = std::regex_replace(line2, std::regex(R"(!\[([^\]]*)\]\(([^\)]+)\))"), "<img src=\"$2\" alt=\"$1\" style=\"max-width:100%;\">");
                // 外链 [text](url)
                line2 = std::regex_replace(line2, std::regex(R"(\[([^\]]+)\]\(([^\)]+)\))"), "<a href=\"$2\" target=\"_blank\" rel=\"noopener noreferrer\">$1</a>");
                // 脚注引用 [^id]
                line2 = std::regex_replace(line2, std::regex(R"(\[\^([a-zA-Z0-9_-]+)\])"), "<sup id=\"ref-$1\"><a href=\"#footnote-$1\">[$1]</a></sup>");
                // 粗体 **text**
                line2 = std::regex_replace(line2, std::regex(R"(\*\*(.+?)\*\*)"), "<b>$1</b>");
                // 斜体 *text*
                line2 = std::regex_replace(line2, std::regex(R"(\*(.+?)\*)"), "<i>$1</i>");
                if (!line2.empty())
                    oss << "<p>" << line2 << "</p>\n";
            }
        }
    }
    // 结束时关闭未闭合的列表
    if (inOl) { oss << "</ol>\n"; inOl = false; }
    if (inUl) { oss << "</ul>\n"; inUl = false; }
    // 输出脚注
    if (!footnoteOrder.empty()) {
        oss << "<hr><section id=\"footnotes\"><ol>\n";
        for (const auto& fn : footnoteOrder) {
            oss << "<li id=\"footnote-" << fn.first << "\">" << fn.second << " <a href=\"#ref-" << fn.first << "\">↩</a></li>\n";
        }
        oss << "</ol></section>\n";
    }
    return oss.str();
}

int main(int argc, char* argv[]) {
    std::string mdFile, htmlFile;

    std::cout << "请输入Markdown文件路径: ";
    std::getline(std::cin, mdFile);
    std::cout << "请输入输出HTML文件路径: ";
    std::getline(std::cin, htmlFile);

    std::ifstream in(mdFile);
    if (!in) {
        std::cerr << "无法打开输入文件: " << mdFile << std::endl;
        return 1;
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();
    std::string markdown = buffer.str();

    std::string html = markdownToHtml(markdown);

    std::ofstream out(htmlFile);
    if (!out) {
        std::cerr << "无法打开输出文件: " << htmlFile << std::endl;
        return 1;
    }
    out << "<!DOCTYPE html>  \n"
        << "<html>  \n"
        << "  \n"
        << "<head>  \n"
        << "    <meta charset=\"UTF-8\">  \n"
        << "    <link rel=\"stylesheet\" href=\"https://cdn.jsdelivr.net/npm/highlight.js@11.9.0/styles/github-dark.min.css\">  \n"
        << "    <script src=\"https://cdn.jsdelivr.net/npm/highlight.js@11.9.0/lib/highlight.min.js\"></script>  \n"
        << "    <script>  \n"
        << "        hljs.highlightAll();  \n"
        << "    </script>  \n"
        << "    <script src=\"https://polyfill.io/v3/polyfill.min.js?features=es6\"></script>  \n"
        << "    <script src=\"https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-mml-chtml.js\"></script>  \n"
        << "    <style>  \n"
        << "        body {  \n"
        << "            font-family: '等线', 'Microsoft YaHei UI', '微软雅黑', 'SimSun', '宋体', sans-serif;  \n"
        << "            background: linear-gradient(120deg, #f8fafc 0%, #e0e7ef 100%);  \n"
        << "            min-height: 100vh;  \n"
        << "            margin: 0;  \n"
        << "            padding: 0;  \n"
        << "        }  \n"
        << "          \n"
        << "        .container {  \n"
        << "            max-width: 800px;  \n"
        << "            margin: 40px auto 40px auto;  \n"
        << "            background: #fff;  \n"
        << "            border-radius: 16px;  \n"
        << "            box-shadow: 0 4px 24px 0 rgba(0, 0, 0, 0.10);  \n"
        << "            padding: 40px 32px 32px 32px;  \n"
        << "        }  \n"
        << "          \n"
        << "        h1 {  \n"
        << "            font-size: 2.2em;  \n"
        << "            color: #2d3a4b;  \n"
        << "            margin-top: 0.5em;  \n"
        << "            margin-bottom: 0.2em;  \n"
        << "            font-weight: 700;  \n"
        << "            letter-spacing: 1px;  \n"
        << "        }  \n"
        << "          \n"
        << "        hr {  \n"
        << "            border: none;  \n"
        << "            border-top: 2px solid #e0e7ef;  \n"
        << "            margin: 1.5em 0 1.5em 0;  \n"
        << "        }  \n"
        << "          \n"
        << "        a,  \n"
        << "        a:visited {  \n"
        << "            color: #2563eb;  \n"
        << "            text-decoration: none;  \n"
        << "            transition: color 0.2s;  \n"
        << "        }  \n"
        << "          \n"
        << "        a:hover {  \n"
        << "            color: #1e40af;  \n"
        << "            text-decoration: underline;  \n"
        << "        }  \n"
        << "          \n"
        << "        .btn {  \n"
        << "            display: inline-block;  \n"
        << "            background: #2563eb;  \n"
        << "            color: #fff !important;  \n"
        << "            border-radius: 6px;  \n"
        << "            padding: 8px 20px;  \n"
        << "            font-size: 1em;  \n"
        << "            margin-bottom: 1em;  \n"
        << "            transition: background 0.2s;  \n"
        << "        }  \n"
        << "          \n"
        << "        .btn:hover {  \n"
        << "            background: #1e40af;  \n"
        << "        }  \n"
        << "          \n"
        << "        ol,  \n"
        << "        ul {  \n"
        << "            padding-left: 2em;  \n"
        << "            margin-bottom: 1.2em;  \n"
        << "        }  \n"
        << "          \n"
        << "        pre {  \n"
        << "            background: #23272e;  \n"
        << "            color: #f8fafc;  \n"
        << "            border-radius: 8px;  \n"
        << "            padding: 1em 1.2em;  \n"
        << "            overflow-x: auto;  \n"
        << "            font-size: 1em;  \n"
        << "            margin-bottom: 1.5em;  \n"
        << "        }  \n"
        << "          \n"
        << "        code {  \n"
        << "            background: #f3f4f6;  \n"
        << "            color: #d6336c;  \n"
        << "            border-radius: 4px;  \n"
        << "            padding: 2px 6px;  \n"
        << "            font-size: 0.98em;  \n"
        << "        }  \n"
        << "          \n"
        << "        pre code {  \n"
        << "            background: none;  \n"
        << "            color: inherit;  \n"
        << "            padding: 0;  \n"
        << "        }  \n"
        << "          \n"
        << "        blockquote {  \n"
        << "            border-left: 4px solid #0c1f47;  \n"
        << "            background: #f1f5fb;  \n"
        << "            color: #374151;  \n"
        << "            margin: 1.2em 0;  \n"
        << "            padding: 0.8em 1.2em;  \n"
        << "            border-radius: 6px;  \n"
        << "        }  \n"
        << "          \n"
        << "        @media (max-width: 600px) {  \n"
        << "            .container {  \n"
        << "                padding: 16px 4vw 16px 4vw;  \n"
        << "            }  \n"
        << "            h1 {  \n"
        << "                font-size: 1.3em;  \n"
        << "            }  \n"
        << "        }  \n"
        << "    </style>  \n"
        << "</head>  \n"
        << "<body style=\"font-family: \'等线\', \'Microsoft YaHei UI\', \'微软雅黑\', \'SimSun\', \'宋体\', sans-serif;\">\n<div class=\"container\">\n"
        << html
        << "<div>\n</body>\n</html>\n";

    std::cout << "转换完成！" << std::endl;
    system("pause");
    return 0;
}