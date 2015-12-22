package info.ephyra.util;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;
import java.net.URLEncoder;

/**
 * Determines the page rank of a URL.
 * 
 * @author Manas Pathak
 * @version 2008-02-10
 */
public class PageRankParser {
	private static int getCheckSum(String url) {
        if (!validUrl(url)) {
            return 0;
		} else {
            return generateCheckSum(strord("info:" + url));
		}
    }
    
    private static String getQueryUrl(String url) {
        int checksum;
        checksum = getCheckSum(url);

		if (checksum == 0) {
            return null;
		}
        
        String temp = "";
        
		try { 
            temp = URLEncoder.encode(url, "UTF-8");
        } catch (UnsupportedEncodingException ex) {
            ex.printStackTrace();
        }
        
		return "http://www.google.com/search?client=navclient-auto&ch=6" + checksum + "&ie=UTF-8&oe=UTF-8&features=Rank" + "&q=info:" + temp;
    }
    
//    private static String getXmlQueryUrl(String url) {
//        int checksum;
//        checksum = getCheckSum(url);
//
//		if (checksum == 0) {
//            return null;
//		}
//        
//        String temp = "";
//
//		try {            
//            temp = URLEncoder.encode(url, "UTF-8");
//        } catch (UnsupportedEncodingException ex) {
//            ex.printStackTrace();
//        }
//        
//        return "http://www.google.com/search?client=navclient-auto&ch=6" + checksum + "&ie=UTF-8&oe=UTF-8" + "&q=info:" + temp;
//    }
    
    private static int[] strord(String str) {
        int result[] = new int[str.length()];
        
		for(int i = 0; i < str.length(); i++) {
            result[i] = str.charAt(i);
		}
        
        return result;
    }
    
    private static int zeroFill(int a, int b) {
        int z = 0x80000000;
        
		if ((z & a) != 0) {
            a >>= 1;
            a &= ~z;
            a |= 0x40000000;
            a >>= b - 1;
        } else {
            a >>= b;
        }
        
		return a;
    }
    
    private static int[] mix(int a, int b, int c) {
        a -= b;
        a -= c;
        a ^= zeroFill(c, 13);
        b -= c;
        b -= a;
        b ^= a << 8;
        c -= a;
        c -= b;
        c ^= zeroFill(b, 13);
        a -= b;
        a -= c;
        a ^= zeroFill(c, 12);
        b -= c;
        b -= a;
        b ^= a << 16;
        c -= a;
        c -= b;
        c ^= zeroFill(b, 5);
        a -= b;
        a -= c;
        a ^= zeroFill(c, 3);
        b -= c;
        b -= a;
        b ^= a << 10;
        c -= a;
        c -= b;
        c ^= zeroFill(b, 15);

        return (new int[] {a, b, c});
    }
    
    private static int generateCheckSum(int url[]) {
        int length = url.length;
        int init = 0xe6359a60;
        int a = 0x9e3779b9;
        int b = 0x9e3779b9;
        int c = init;
        int k = 0;
        int len;
        int mix[];
        
		for(len = length; len >= 12; len -= 12) {
            a += url[k + 0] + (url[k + 1] << 8) + (url[k + 2] << 16) + (url[k + 3] << 24);
            b += url[k + 4] + (url[k + 5] << 8) + (url[k + 6] << 16) + (url[k + 7] << 24);
            c += url[k + 8] + (url[k + 9] << 8) + (url[k + 10] << 16) + (url[k + 11] << 24);
        
			mix = mix(a, b, c);
            
			a = mix[0];
            b = mix[1];
            c = mix[2];
            k += 12;
        }
        
        c += length;

        switch (len) {
            case 11: // '\013'
                c += url[k + 10] << 24;
                // fall through
                
            case 10: // '\n'
                c += url[k + 9] << 16;
                // fall through
                
            case 9: // '\t'
                c += url[k + 8] << 8;
                // fall through
                
            case 8: // '\b'
                b += url[k + 7] << 24;
                // fall through
                
            case 7: // '\007'
                b += url[k + 6] << 16;
                // fall through
                
            case 6: // '\006'
                b += url[k + 5] << 8;
                // fall through
                
            case 5: // '\005'
                b += url[k + 4];
                // fall through
                
            case 4: // '\004'
                a += url[k + 3] << 24;
                // fall through
                
            case 3: // '\003'
                a += url[k + 2] << 16;
                // fall through
                
            case 2: // '\002'
                a += url[k + 1] << 8;
                // fall through
                
            case 1: // '\001'
                a += url[k + 0];
                // fall through
                
            default:
                mix = mix(a, b, c);
                break;
        }
        
		return mix[2];
    }

    
    private static boolean validUrl(String url) {
        if (url == null || !url.startsWith("http")) {
            return false;
		}

        try {
            new URL(url);
        } catch(MalformedURLException e) {
            return false;
        }
        
		return true;
    }

	public static int getPageRank(String url) {
        int pageRank = -1;
        String query = getQueryUrl(url);

		if (query == null) {
            return pageRank;
		}

        BufferedReader in = null;
        
		try {
            URL pr = new URL(query);
            URLConnection conn = pr.openConnection();
            in = new BufferedReader(new InputStreamReader(conn.getInputStream()));
            String line = null;

			do {
                if ((line = in.readLine()) == null) {
                    break;
				}

                if (line.contains(":")) {
                    String tokens[] = line.split(":");
                    if(tokens.length > 2)
                        pageRank = Integer.parseInt(tokens[2]);
                }
            } while(true);
        } catch(Exception e) { 
		}
        
		return pageRank;
    }
    
	public static void main(String[] args) {
		int pr = PageRankParser.getPageRank(args[0]);
		System.out.println(pr);
	}
}
