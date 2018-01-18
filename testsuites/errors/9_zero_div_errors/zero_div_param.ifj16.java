class Main {
    static int a = 5;
    static void run() {
        foo(a);
    }
    static void foo(int a) {
        a = a/0;
    }
}
