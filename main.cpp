#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#include <stack>

using namespace std;

#define PAGE_SIZE 512


struct Alumno {
    char codigo[3];
    char nombre[7];
    int ciclo;

    friend bool operator==(char *key, const Alumno &alumno2) {
        return strcmp(key, alumno2.codigo) == 0;
    }

    friend bool operator<(char *key, const Alumno &alumno2) {
        return strcmp(key, alumno2.codigo) < 0;
    }

    friend bool operator>(char *key, const Alumno &alumno2) {
        return strcmp(key, alumno2.codigo) > 0;
    }
};


// PAGE_SIZE = sizeof(KeyType) * M + sizeof(long) * (M+1) + sizeof(int)
template<typename KeyType>
const long M = (PAGE_SIZE - sizeof(int) - sizeof(long)) / (sizeof(KeyType) + sizeof(long));

#define M M<KeyType>

template<typename KeyType>
struct IndexPage {
    int size = 0;
    KeyType keys[M];
    long children[M + 1];

    IndexPage() = default;

    long locate(KeyType key) {
        int i = 0;
        while (i < size && key > keys[i]) {
            ++i;
        }
        return children[i];
    }
};

// PAGE_SIZE = sizeof(RecordType) * N + sizeof(int) + sizeof(long)
template<typename RecordType>
const long N = (PAGE_SIZE - sizeof(int) - sizeof(long)) / sizeof(RecordType);

#define N N<RecordType>

template<typename RecordType, typename KeyType>
struct DataPage {
    int size = 0;
    RecordType records[N];
    long next = -1;

    DataPage() = default;

    void insert(fstream &file, long firstPage, RecordType record) {
        // there are two options, we either
        // 1. find a non-full page in the linked list of pages, in which case we insert it to the end of the non-full page and re-write it
        // 2. all pages are full, in which case we have to create a new page in the linked list of pages
        long currentPagePos = firstPage;
        while (true) {
            // non-full page, insert element
            if (size < N) {
                records[size++] = record;
                file.seekp(currentPagePos);
                file.write((char *) this, PAGE_SIZE);
                break;
            } else {
                // all pages are full, create new page and link it to list
                if (next == -1) {
                    // create new page with 1 record
                    DataPage<RecordType, KeyType> newPage{};
                    newPage.records[0] = record;
                    newPage.size = 1;
                    newPage.next = -1;
                    file.seekp(0, ios::end);
                    // link to new page
                    this->next = file.tellp();
                    file.write((char *) &newPage, PAGE_SIZE);
                    // update current page
                    file.seekp(currentPagePos);
                    file.write((char *) &newPage, PAGE_SIZE);
                    break;
                }
                // we can still explore the linked list of pages
                else {
                    currentPagePos = next;
                    file.seekp(next);
                    file.read((char *) this, PAGE_SIZE);
                }
            }
        }
    }

    vector<RecordType> locate(fstream &file, KeyType key) {
        vector<RecordType> result;
        while (true) {
            for (int i = 0; i < size; ++i) {
                if (key == records[i]) {
                    result.push_back(records[i]);
                }
            }
            if (next != -1) {
                file.seekg(next);
                file.read((char *) this, PAGE_SIZE); // for all the overflow pages that has our page where we landed, there will be a lecture.
            } else {
                break;
            }
        }
        return result;
    }

    vector<RecordType> locateRange(fstream &file, KeyType begin_key, KeyType end_key, long firstPosPage) {
        vector<RecordType> result;
        stack<long> overflowPages;
        int temp = 0;
        while (temp < size && begin_key > records[temp]) ++temp;

        bool flag = true;
        for (int j = temp; j < sizeof(records); ++j) {
            if (end_key < records[j]) {
                flag = false;
                break;
            }
            result.push_back(records[j]);
        }

        if(next != -1 && flag) overflowPages.push(next);

        int pagesRead = 1;
        DataPage<RecordType, KeyType> dataPage;
        while (flag) {
            file.seekg(firstPosPage + PAGE_SIZE*pagesRead++);
            file.read((char *) &dataPage, PAGE_SIZE); // lecture for all remaining pages that key is in range
            for (int i = 0; i < sizeof(dataPage.records); ++i) {
                if (end_key < dataPage.records[i]) {
                    flag = false;
                    break;
                }
                result.push_back(dataPage.records[i]);
            }
            if(next != -1 && flag) overflowPages.push(dataPage.next);

        }

        while(!overflowPages.empty()){
            file.seekg(overflowPages.top());
            file.read((char *) &dataPage, PAGE_SIZE); // for each identified overflow page where is our key in range, there will be a lecture
            for (int i = 0; i < sizeof(dataPage); ++i) {
                if(pagesRead == 1) {
                    if (end_key < dataPage.records[i]) result.push_back(dataPage.records[i]);
                }
                else
                    result.push_back(dataPage.records[i]);
            }
            overflowPages.pop();
        }

        return result;
    }
};

template<typename RecordType, typename KeyType>
class ISAM {
    fstream file;
    string filename;
    fstream index1;
    fstream index2;
    fstream index3;
    string indexfilename(int number) {
        return "index" + to_string(number) + ".dat";
    }
    _Ios_Openmode flags = ios::out | ios::app | ios::binary;

public:
    ISAM(const string &filename) : filename(filename) {
        file.open(filename, flags);
        file.close();
    }
    void add(RecordType record) {
        // read index file 1
        index1.open(indexfilename(1), flags);
        IndexPage<KeyType> indexPage1;
        index1.read((char *) &indexPage1, PAGE_SIZE);
        long page1 = indexPage1.locate(record.codigo);
        index1.close();

        // read index file 2
        index2.open(indexfilename(2), flags);
        index2.seekg(page1);
        IndexPage<KeyType> indexPage2;
        index2.read((char *) &indexPage2, PAGE_SIZE);
        long page2 = indexPage2.locate(record.codigo);
        index2.close();

        // read index file 3
        index3.open(indexfilename(3), flags);
        index3.seekg(page2);
        IndexPage<KeyType> indexPage3;
        index3.read((char *) &indexPage3, PAGE_SIZE);
        long page3 = indexPage3.locate(record.codigo);
        index3.close();

        file.open(filename, flags);
        file.seekg(page3);
        DataPage<RecordType, KeyType> dataPage;
        file.read((char *) &dataPage, PAGE_SIZE);
        dataPage.insert(file, page3, record);
        file.close();
    }
    vector<RecordType> search(KeyType key) {
        // read index file 1
        index1.open(indexfilename(1), flags);
        IndexPage<KeyType> indexPage1;
        index1.read((char *) &indexPage1, PAGE_SIZE); // Lectura O(1) del disco
        long page1 = indexPage1.locate(key);
        index1.close();

        // read index file 2
        index2.open(indexfilename(2), flags);
        index2.seekg(page1);
        IndexPage<KeyType> indexPage2;
        index2.read((char *) &indexPage2, PAGE_SIZE); // Lectura O(1) del disco
        long page2 = indexPage2.locate(key);
        index2.close();

        // read index file 3
        index3.open(indexfilename(3), flags);
        index3.seekg(page2);
        IndexPage<KeyType> indexPage3;
        index3.read((char *) &indexPage3, PAGE_SIZE); // Lectura O(1) del disco
        long page3 = indexPage3.locate(key);
        index3.close();

        file.open(filename, flags);
        file.seekg(page3);
        DataPage<RecordType, KeyType> dataPage;
        file.read((char *) &dataPage, PAGE_SIZE);
        vector<RecordType> records = dataPage.locate(file, key); // Lectura O(1) del disco solo si key se encuentra en un overflow page. Se repite el proceso hasta encontrarlo entre las overflow pages existentes secuenciales.
        file.close();
        return records;
    }


    vector<RecordType> rangeSearch(KeyType begin_key, KeyType end_key) { // O(4 + m + k) lecturas del disco, siendo m la cantidad de paginas adicionales leidas y siendo k la cantidad de overflow pages adicionales leidas en donde esta el resultado.
        // read index file 1
        index1.open(indexfilename(1), flags);
        IndexPage<KeyType> indexPage1;
        index1.read((char *) &indexPage1, PAGE_SIZE); // Lectura O(1) del disco
        long page1 = indexPage1.locate(begin_key);
        index1.close();

        // read index file 2
        index2.open(indexfilename(2), flags);
        index2.seekg(page1);
        IndexPage<KeyType> indexPage2;
        index2.read((char *) &indexPage2, PAGE_SIZE); // Lectura O(1) del disco
        long page2 = indexPage2.locate(begin_key);
        index2.close();

        // read index file 3
        index3.open(indexfilename(3), flags);
        index3.seekg(page2);
        IndexPage<KeyType> indexPage3;
        index3.read((char *) &indexPage3, PAGE_SIZE); // Lectura O(1) del disco
        long page3 = indexPage3.locate(begin_key);
        index3.close();

        file.open(filename, flags);
        file.seekg(page3);
        DataPage<RecordType, KeyType> dataPage;
        file.read((char *) &dataPage, PAGE_SIZE); // Lectura O(1) del disco
        vector<RecordType> records = dataPage.locateRange(file, begin_key, end_key, page3); // Lectura O(1) del disco por cada page y overflow page leída en donde esta el resultado de la key acotada.
        file.close();
        return records;
    }
};

int main() {
    ISAM<Alumno, char[10]> isam("data.dat");
    auto result = isam.search("20 ");
    for (const auto &a: result) {
        cout << a.codigo << " " << a.nombre << " " << a.ciclo << endl;
    }
    auto range = isam.rangeSearch("20 ", "200");
    for (const auto &a: range) {
        cout << a.codigo << " " << a.nombre << " " << a.ciclo << endl;
    }
    Alumno nuevo{};
    strcpy(nuevo.codigo, "192");
    strcpy(nuevo.nombre, "Joaquin");
    nuevo.ciclo = 3;
    isam.add(nuevo);
    return 0;
}
