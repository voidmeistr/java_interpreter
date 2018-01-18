class Main {
    static void run() {
        foo(10);
    }
    static int foo(int c) {
        int dec = c - 1;
        int p;
        if ( c > 0) {
            foo(dec);
            p = dec;
            p = Rec.foo(dec);
            ifj16.print(p + "\n");
        }
        return 1;
    }
}

class Rec {
    static int a = 0;
    static int foo(double p) {
        a = a + 1;
        return a;
    }
}
