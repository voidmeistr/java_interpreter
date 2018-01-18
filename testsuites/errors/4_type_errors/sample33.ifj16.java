class Main {
    static void run() {
        sec.run(4);   
    }
}

class sec {
    static double run(int a) {
        String c = "str";
        return c; // returns string instead of double
    }
}