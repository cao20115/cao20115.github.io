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
        if (std::regex_match(line, match, std::regex(R"(^\s*#\s*(.*))"))) {
            oss << "<h1>" << match[1] << "</h1><hr>\n";
        } else if (std::regex_match(line, match, std::regex(R"(^\s*##\s*(.*))"))) {
            oss << "<h2>" << match[1] << "</h2><hr>\n";
        } else if (std::regex_match(line, match, std::regex(R"(^\s*###\s*(.*))"))) {
            oss << "<h3>" << match[1] << "</h3><hr>\n";
        } else {
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
    out << "<!DOCTYPE html>\n<html>\n<head>\n"
        << "<meta charset=\"UTF-8\">\n"
        << "<link rel=\"stylesheet\" href=\"https://cdn.jsdelivr.net/npm/highlight.js@11.9.0/styles/github-dark.min.css\">\n"
        << "<script src=\"https://cdn.jsdelivr.net/npm/highlight.js@11.9.0/lib/highlight.min.js\"></script>\n"
        << "<script>hljs.highlightAll();</script>\n"
        << "<script src=\"https://polyfill.io/v3/polyfill.min.js?features=es6\"></script>\n"
        << "<script src=\"https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-mml-chtml.js\"></script>\n"
        << "<style>body{font-family:sans-serif;padding:2em;}</style>\n"
        << "</head>\n<body style=\"font-family: \'等线\', \'Microsoft YaHei UI\', \'微软雅黑\', \'SimSun\', \'宋体\', sans-serif;\">\n" << html << "</body>\n</html>\n";

    std::cout << "转换完成！" << std::endl;
    system("pause");
    return 0;
}