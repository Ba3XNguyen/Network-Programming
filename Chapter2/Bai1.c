#include<stdio.h>
#include <string.h>
#include<math.h>
struct student {
    char name[20];
    int eng;
    int math;
    int phys;
    double mean;
};
static struct student data[]={
    {"Tuan", 82, 72, 58, 0.0},
    {"Nam", 77, 82, 79, 0.0},
    {"Khanh", 52, 62, 39, 0.0},
    {"Phuong", 61, 82, 88, 0.0}
};
int n = sizeof(data)/sizeof(data[0]);
void part_a( struct student data[]){
    for (int i=0;i<n;i++){
        data[i].mean= (data[i].eng+data[i].math+data[i].phys)/3.0;
    }
    for (int i=0;i<4;i++){
        printf("%s %d %d %d %.2lf\n", data[i].name, data[i].eng, data[i].math, data[i].phys, data[i].mean);
    }
}
void part_b(struct student data[]){
    for (int i=0;i<n;i++){
        data[i].mean= (data[i].eng+data[i].math+data[i].phys)/3.0;
    }   
    for (int i=0;i<n;i++){
        printf("%s %.2lf grade " , data[i].name, data[i].mean);
        if (data[i].mean<60) printf("D");
        if (60<=data[i].mean&& data[i].mean<70) printf("C");
        if (70<=data[i].mean && data[i].mean<80) printf ("B");
        if (80<=data[i].mean && data[i].mean <90) printf("A");
        if (90<=data[i].mean) printf("S");
        printf("\n");
    }
}
int main (){
    printf ("Part a: \n");
    part_a(data);
    printf("Part b:\n");
    part_b(data);
    return 0;
}