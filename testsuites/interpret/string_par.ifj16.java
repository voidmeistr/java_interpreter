class Main {
    static void run() {
        String str = "bca";
        str = str_expand(str, 10);
        ifj16.print(str + "\n");
    }
    static String str_expand(String str, int exp ) {
        String str1 = ifj16.sort(str);
        String str2 = str + str1;
        String str3 = str1 + str;

        if (exp > 0) {
            exp = exp - 1;
            str = str1 + str2 + str3;
            str = str_expand(str, exp);
            return str;
        }
        return str;
    }
}
