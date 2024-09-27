#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <ctime>      // Для работы с датой и временем
#include <iomanip>    // Для форматирования даты и времени
#include <fstream>    // Для работы с файлами

using namespace std;

// Объявление функций
void printError(const std::string& errorMessage);
std::string pwd();
std::string ls();
std::string cat(const std::string& filename);
std::string date();
void logAction(const std::string& user, const std::string& action, const std::string& output = "");

class Catalog;

// Глобальные переменные
Catalog* root;
Catalog* currentDirectory;
std::ofstream logFile;

class FileSystemItem {
public:
    string path;
    Catalog* parent;

    FileSystemItem(string path, Catalog* parent = nullptr);

    virtual ~FileSystemItem();

    virtual bool isCatalog() const;

    virtual Catalog* asCatalog();

    virtual string getFullPath() const;
};

class Catalog : public FileSystemItem {
public:
    vector<FileSystemItem*> items;

    Catalog(string path, Catalog* parent = nullptr);

    virtual ~Catalog();

    void addItem(FileSystemItem* item);

    FileSystemItem* getItem(string relativePath);

    virtual bool isCatalog() const override;

    virtual Catalog* asCatalog() override;
};

// Реализация методов FileSystemItem

FileSystemItem::FileSystemItem(string path, Catalog* parent)
    : path(path), parent(parent) {}

FileSystemItem::~FileSystemItem() {}

bool FileSystemItem::isCatalog() const {
    return false;
}

Catalog* FileSystemItem::asCatalog() {
    return nullptr;
}

string FileSystemItem::getFullPath() const {
    if (parent == nullptr || parent->path.empty()) {
        return "/" + path;
    }
    else if (parent->parent == nullptr || parent->parent->path.empty()) {
        return "/" + parent->path + "/" + path;
    }
    else {
        return parent->getFullPath() + "/" + path;
    }
}

// Реализация методов Catalog
Catalog::Catalog(string path, Catalog* parent)
    : FileSystemItem(path, parent) {}

Catalog::~Catalog() {
    for (FileSystemItem* item : items) {
        delete item;
    }
    items.clear();
}

void Catalog::addItem(FileSystemItem* item) {
    item->parent = this;
    items.push_back(item);
}

FileSystemItem* Catalog::getItem(string relativePath) {
    if (relativePath.empty()) {
        return nullptr;
    }

    if (relativePath[0] == '/') {
        // Абсолютный путь, начинаем поиск с корня
        Catalog* rootCatalog = this;
        while (rootCatalog->parent != nullptr) {
            rootCatalog = rootCatalog->parent;
        }
        return rootCatalog->getItem(relativePath.substr(1));
    }

    size_t pos = relativePath.find('/');
    string itemName;
    string subPath;

    if (pos == string::npos) {
        itemName = relativePath;
        subPath = "";
    }
    else {
        itemName = relativePath.substr(0, pos);
        subPath = relativePath.substr(pos + 1);
    }

    for (FileSystemItem* item : items) {
        if (item->path == itemName) {
            if (subPath.empty()) {
                return item;
            }
            else if (item->isCatalog()) {
                return item->asCatalog()->getItem(subPath);
            }
            else {
                return nullptr;
            }
        }
    }

    return nullptr;
}

bool Catalog::isCatalog() const {
    return true;
}

Catalog* Catalog::asCatalog() {
    return this;
}

// Класс File
class File : public FileSystemItem {
public:
    std::string content;

    File(string path, const std::string& content = "", Catalog* parent = nullptr);

    virtual ~File() override;
};

File::File(string path, const std::string& content, Catalog* parent)
    : FileSystemItem(path, parent), content(content) {}

File::~File() {}

int main() {
    setlocale(LC_ALL, "Russian");
    // Инициализация root
    root = new Catalog("");
    currentDirectory = root;

    // Создаем каталоги /home и /home/user
    Catalog* home = new Catalog("home", root);
    root->addItem(home);

    Catalog* user = new Catalog("user", home);
    home->addItem(user);

    // Устанавливаем текущий каталог на /home/user
    currentDirectory = user;

    // Создаем файлы и добавляем их в текущий каталог
    File* file1 = new File("text1.txt", "Содержимое файла text1.txt", currentDirectory);
    File* file2 = new File("text2.txt", "Содержимое файла text2.txt", currentDirectory);
    File* file3 = new File("text3.txt", "Содержимое файла text3.txt", currentDirectory);

    currentDirectory->addItem(file1);
    currentDirectory->addItem(file2);
    currentDirectory->addItem(file3);

    // Открываем лог-файл
    logFile.open("emulator_log.csv", std::ios::out | std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "Error opening log file." << std::endl;
        return 1;
    }

    std::string input;

    while (true) {
        // Приглашение командной строки
        std::cout << "user@" << currentDirectory->getFullPath() << "# ";
        std::getline(std::cin, input);

        if (input == "pwd") {
            std::string output = pwd();
            std::cout << output << std::endl;
            logAction("user", "pwd", output);
        }
        else if (input == "ls") {
            std::string output = ls();
            std::cout << output;
            logAction("user", "ls", output);
        }
        else if (input.substr(0, 4) == "cat ") {
            std::string filename = input.substr(4);
            std::string output = cat(filename);
            std::cout << output << std::endl;
            logAction("user", "cat " + filename, output);
        }
        else if (input == "date") {
            std::string output = date();
            std::cout << output << std::endl;
            logAction("user", "date", output);
        }
        else if (input == "exit") {
            logAction("user", "exit");
            break;
        }
        else {
            std::string errorMsg = "Command not found.";
            printError(errorMsg);
            logAction("user", "unknown command: " + input, errorMsg);
        }
    }

    // Закрываем лог-файл
    logFile.close();

    // Освобождаем память
    delete root;

    return 0;
}

void printError(const std::string& errorMessage) {
    std::cerr << errorMessage << std::endl;
}

std::string pwd() {
    return currentDirectory->getFullPath();
}

std::string ls() {
    std::stringstream ss;
    for (FileSystemItem* item : currentDirectory->asCatalog()->items) {
        ss << item->path << std::endl;
    }
    return ss.str();
}

std::string cat(const std::string& filename) {
    FileSystemItem* item = currentDirectory->getItem(filename);

    if (item) {
        if (!item->isCatalog()) {
            File* file = dynamic_cast<File*>(item);
            if (file) {
                return file->content;
            }
            else {
                return "Error reading file.";
            }
        }
        else {
            return "Cannot display content of a directory.";
        }
    }
    else {
        return "File not found.";
    }
}

std::string date() {
    // Получаем текущее время
    std::time_t now = std::time(nullptr);
    std::tm localTime;

    if (localtime_s(&localTime, &now) != 0) {
        return "Error getting current date and time.";
    }

    // Форматируем дату и время
    std::stringstream ss;
    ss << std::put_time(&localTime, "%c");
    return ss.str();
}

void logAction(const std::string& user, const std::string& action, const std::string& output) {
    // Проверяем, что лог-файл открыт
    if (!logFile.is_open()) {
        return;
    }

    // Получаем текущую дату и время
    std::time_t now = std::time(nullptr);
    std::tm localTime;

    if (localtime_s(&localTime, &now) != 0) {
        // Обработка ошибки
        return;
    }

    // Форматируем дату и время в строку
    char timeBuffer[20]; // Формат: YYYY-MM-DD HH:MM:SS
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &localTime);

    // Записываем запись в лог-файл в формате CSV
    logFile << "\"" << timeBuffer << "\","
        << "\"" << user << "\","
        << "\"" << action << "\","
        << "\"" << output << "\"\n";

    // Обеспечиваем немедленную запись в файл
    logFile.flush();
}
