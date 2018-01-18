class Main {
    static void run() {
        int a;
        int vysl;
        int neg;
        a = 7;
        if (a < 0) {
            ifj16.print("neda sa\n");
        }
        else {
            vysl = fact(a);
            neg = 0 - vysl;
            ifj16.print("vysledok je:" + neg + "\n");
            ifj16.print("vysledok je:" + vysl + "\n");
        }
    }
    static int ret() {return 5;}
    static int fact(int n) {
        int tmp;
        int dec = n -1;
        if ( 2 > n) {
            return 1;
        }else {
            tmp = fact(dec);
            tmp = n * tmp;
            return tmp;
        }
    }
}
