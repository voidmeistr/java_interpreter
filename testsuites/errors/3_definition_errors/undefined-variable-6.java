class Main {
    static int a;
    static int b;
    static int c;
    static void run() {

        bar.foo(a, b);
    }
}

class bar {
    static void foo(int a, int b) {
        d=a; //using undeclared parameter in called function
    }
}
