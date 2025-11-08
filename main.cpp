#include <bits/stdc++.h>
using namespace std;

struct Task {
    int id;
    string title;
    bool done = false;
};

static const string CSV_FILE = "tasks.csv";

// --- CSV utilities (最小限のエスケープ対応) ---
string csv_escape(const string& s) {
    bool need_quote = s.find_first_of(",\"\n\r") != string::npos;
    string out = s;
    // " を "" にエスケープ
    size_t pos = 0;
    while ((pos = out.find('"', pos)) != string::npos) {
        out.insert(pos, 1, '"');
        pos += 2;
    }
    if (need_quote) return "\"" + out + "\"";
    return out;
}
bool csv_unescape_line(const string& line, vector<string>& fields) {
    fields.clear();
    string cur; bool in_quote = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (in_quote) {
            if (c == '"') {
                // 連続する "" は " として扱う
                if (i + 1 < line.size() && line[i+1] == '"') {
                    cur.push_back('"'); ++i;
                } else {
                    in_quote = false;
                }
            } else {
                cur.push_back(c);
            }
        } else {
            if (c == '"') {
                in_quote = true;
            } else if (c == ',') {
                fields.push_back(cur); cur.clear();
            } else {
                cur.push_back(c);
            }
        }
    }
    if (in_quote) return false; // 不正（閉じてない）
    fields.push_back(cur);
    return true;
}

// --- 永続化 ---
vector<Task> load_tasks() {
    vector<Task> tasks;
    ifstream fin(CSV_FILE);
    if (!fin) return tasks;
    string line;
    while (getline(fin, line)) {
        if (line.empty()) continue;
        vector<string> cols;
        if (!csv_unescape_line(line, cols)) continue;
        if (cols.size() < 3) continue;
        Task t;
        try {
            t.id = stoi(cols[0]);
            t.title = cols[1];
            t.done = (cols[2] == "1" || cols[2] == "true");
            tasks.push_back(t);
        } catch (...) { /* skip bad row */ }
    }
    return tasks;
}

void save_tasks(const vector<Task>& tasks) {
    ofstream fout(CSV_FILE, ios::trunc);
    for (auto& t : tasks) {
        fout << t.id << ","
             << csv_escape(t.title) << ","
             << (t.done ? "1" : "0") << "\n";
    }
}

// --- ユーティリティ ---
int next_id(const vector<Task>& tasks) {
    int mx = 0;
    for (auto& t : tasks) mx = max(mx, t.id);
    return mx + 1;
}
optional<size_t> find_index_by_id(const vector<Task>& tasks, int id) {
    for (size_t i = 0; i < tasks.size(); ++i)
        if (tasks[i].id == id) return i;
    return nullopt;
}

void print_list(const vector<Task>& tasks) {
    if (tasks.empty()) {
        cout << "(タスクなし)\n";
        return;
    }
    // id昇順で表示
    vector<Task> sorted = tasks;
    sort(sorted.begin(), sorted.end(), [](const Task& a, const Task& b){
        return a.id < b.id;
    });
    for (auto& t : sorted) {
        cout << "[" << t.id << "] "
             << t.title << "   (" << (t.done ? "完了" : "未完") << ")\n";
    }
}

void print_help(const string& exe) {
    cout << "使い方:\n";
    cout << "  " << exe << " add <title>      : タスク追加（タイトルは\"で囲うと空白OK）\n";
    cout << "  " << exe << " list             : 一覧\n";
    cout << "  " << exe << " done <id>        : 完了にする\n";
    cout << "  " << exe << " undone <id>      : 未完に戻す\n";
    cout << "  " << exe << " remove <id>      : 削除\n";
    cout << "  " << exe << " clear-done       : 完了タスクを一括削除\n";
    cout << "  " << exe << " help             : このヘルプ\n";
}

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    vector<Task> tasks = load_tasks();

    string exe = (argc > 0 ? string(argv[0]) : "todo");
    if (argc <= 1) {
        print_help(exe);
        return 0;
    }
    string cmd = argv[1];

    if (cmd == "add") {
        if (argc <= 2) {
            cerr << "エラー: タイトルを指定してください\n";
            return 1;
        }
        // 2番目以降を結合（空白対応）
        string title;
        for (int i = 2; i < argc; ++i) {
            if (i > 2) title += " ";
            title += argv[i];
        }
        Task t;
        t.id = next_id(tasks);
        t.title = title;
        t.done = false;
        tasks.push_back(t);
        save_tasks(tasks);
        cout << "追加: [" << t.id << "] " << t.title << "\n";
    }
    else if (cmd == "list") {
        print_list(tasks);
    }
    else if (cmd == "done" || cmd == "undone") {
        if (argc <= 2) {
            cerr << "エラー: id を指定してください\n";
            return 1;
        }
        int id = stoi(argv[2]);
        auto idx = find_index_by_id(tasks, id);
        if (!idx) {
            cerr << "エラー: id=" << id << " が見つかりません\n";
            return 1;
        }
        tasks[*idx].done = (cmd == "done");
        save_tasks(tasks);
        cout << "更新: [" << id << "] を " << (cmd=="done" ? "完了" : "未完") << " にしました\n";
    }
    else if (cmd == "remove") {
        if (argc <= 2) {
            cerr << "エラー: id を指定してください\n";
            return 1;
        }
        int id = stoi(argv[2]);
        auto idx = find_index_by_id(tasks, id);
        if (!idx) {
            cerr << "エラー: id=" << id << " が見つかりません\n";
            return 1;
        }
        cout << "削除: [" << tasks[*idx].id << "] " << tasks[*idx].title << "\n";
        tasks.erase(tasks.begin() + *idx);
        save_tasks(tasks);
    }
    else if (cmd == "clear-done") {
        size_t before = tasks.size();
        tasks.erase(remove_if(tasks.begin(), tasks.end(),
                              [](const Task& t){ return t.done; }),
                    tasks.end());
        save_tasks(tasks);
        cout << "完了タスクを削除: " << (before - tasks.size()) << " 件\n";
    }
    else if (cmd == "help") {
        print_help(exe);
    }
    else {
        cerr << "不明なコマンド: " << cmd << "\n";
        print_help(exe);
        return 1;
    }

    return 0;
}
