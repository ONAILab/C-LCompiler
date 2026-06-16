
int sub(int a, int b) {
    return a - b;
}


void print_result(int result) {
    printf("结果是: %d\n", result);
}

int* create_linked_list(int size) {
    int* list = (int*)malloc(size * sizeof(int));
    for (int i = 0; i < size; i++) {
        list[i] = i + 1; // 初始化链表元素
    }
    return list;
}



void main() {
    int x;
    int y;
    int result;
    
    x = 10;
    y = 5;
    
    
    result = sub(x, y);
    print_result(result);
    
}




