class Main {
    static void run() {
        int a;
        int b;
        if (a == b) {
            while (a > b) {
                a = a + 1;
            }
        }
    }
}

class bar {
    static int c;
    class foo { // nested class
        static int k;
    }
}
