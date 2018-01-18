class Main {
    static void run() {
        sec.run("str");   // str instead of double
    }
}

class sec {
    static double run(double a) {
        return a;
    }
}