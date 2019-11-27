import java.io.FileInputStream;
import java.security.cert.*;
import java.util.*;
public class x509 {

	public static void getX509Certificate(String path) throws Exception {
		X509Certificate cer = null;
		CertificateFactory factory = CertificateFactory.getInstance("X.509");
		FileInputStream file = new FileInputStream(path);
		cer = (X509Certificate)factory.generateCertificate(file);
		file.close();
		
		System.out.println("读取x.509证书信息...");
		System.out.println("序列号			:"+cer.getSerialNumber());
		System.out.println("发布方标识名	 	:"+cer.getIssuerDN()); 
		System.out.println("主体标识	    	:"+cer.getSubjectDN());
		System.out.println("证书算法OID字符串	:"+cer.getSigAlgOID());
		System.out.println("证书有效期		:"+cer.getNotAfter());
		System.out.println("签名算法		:"+cer.getSigAlgName());
		System.out.println("版本号			:"+cer.getVersion());
		System.out.println("公钥			:"+cer.getPublicKey());
	}
	
	public static void main(String[] args) throws Exception {
		Scanner input = new Scanner(System.in);
		System.out.print("input path of certificate you want to analysize:\n");
		String path = input.next();
		getX509Certificate(path);
		input.close();
	}
}
