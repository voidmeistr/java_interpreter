class Main {
    static void run() {
        int a;
        int b;
        foo();  // undefined - bar.foo()
    }
}

class bar {
    static void foo() {}
}
