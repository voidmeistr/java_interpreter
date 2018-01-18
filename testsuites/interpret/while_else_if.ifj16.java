class Main {
    static int cnt = 20;
    static String str = "";
    static void run() {
        String str1 = "fdasfa"; // clas2.str
        String str2 = "fda";    // clas3.str
        String str = "gfgs";    // Main.str
        ifj16.print(clas2.str + "==" +clas3.str +"\n");
        int i = ifj16.compare(clas2.str, clas3.str);
        if ( i > 0) {
            while (cnt >= 0) {
                ifj16.print(clas2.str + "\n");
                cnt = cnt - 2;
            }
        } else if (i < 0) {
             while (cnt >= 0) {
                str = str2 + cnt + " ";
                ifj16.print(str + "\n");
                cnt = cnt - 5;
            }
        }
        cnt = 5;
        while (cnt <= 9) {
            if (cnt == 5) {
                ifj16.print("5\n");
            }
            else if(cnt == 6) {
                ifj16.print("5a\n");
            }
            else if (cnt == 7) {
                ifj16.print("7\n");
            }
            else if (cnt == 8) {
                ifj16.print("p7\n");
            }
            cnt = cnt + 1;
        }
    }
}

class clas2 {
    static String str = "fdsgga";
}

class clas3 {
    static String str = "fadf";
}
