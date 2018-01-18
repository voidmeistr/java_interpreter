class Main {
    static void run() {
        int a1 = 0;
        int a2 = 1;
        int count = 10;
        int tmp;
        while ( count > 0 ) {
            tmp = a2;
            a2 = a2 + a1;
            a1 = tmp;
            ifj16.print(a2 + "\n");
            count = count - 1;
        }
    }
}
