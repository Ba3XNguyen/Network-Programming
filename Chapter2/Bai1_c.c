#include <stdio.h>

typedef struct student{
    char name[20];
    int eng;
    int math;
    int phys;
}STUDENT;

STUDENT data[]={
    {"Tuan", 82, 72, 58},
    {"Nam", 77, 82, 79},
    {"Khanh", 52, 62, 39},
    {"Phuong", 61, 82, 88}
};

int main(){

    STUDENT *p;
    int n = sizeof(data)/sizeof(data[0]);

    for(p = data; p < data + n; p++){
        printf("%s %d %d %d\n",
               p->name,
               p->eng,
               p->math,
               p->phys);
    }

    return 0;
}