package io.github.mrcalzon02.reverievr;

import com.hierynomus.msfscc.fileinformation.FileIdBothDirectoryInformation;
import com.hierynomus.msdtyp.AccessMask;
import com.hierynomus.smbj.SMBClient;
import com.hierynomus.smbj.SmbConfig;
import com.hierynomus.smbj.auth.AuthenticationContext;
import com.hierynomus.smbj.connection.Connection;
import com.hierynomus.smbj.session.Session;
import com.hierynomus.smbj.share.DiskShare;
import com.hierynomus.smbj.share.File;
import com.hierynomus.mssmb2.SMB2CreateDisposition;
import com.hierynomus.mssmb2.SMB2CreateOptions;
import com.hierynomus.mssmb2.SMB2ShareAccess;

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.EnumSet;
import java.util.List;
import java.util.concurrent.TimeUnit;

/** Windows SMB2/SMB3 read-only source, used only on the Stage A import worker. */
final class LaptopSmbClient implements AutoCloseable {
    private static final int MAX_ENTRIES=200;
    static final class Entry {
        final String name;
        final String path;
        final boolean directory;
        final long size;

        Entry(String name,String path,boolean directory,long size) {
            this.name=name;
            this.path=path;
            this.directory=directory;
            this.size=size;
        }
        String label() {
            return directory ? "[DIR] "+name
                : name + " (" + Math.max(0L,size/1024L) + " KiB)";
        }
    }

    private final String host, share, user, domain;
    private final char[] password;
    private volatile boolean closed;

    LaptopSmbClient(String host,String share,String user,String domain,
                    char[] password) throws IOException {
        this.host=LaptopSharePath.host(host==null?null:host.trim());
        this.share=LaptopSharePath.share(share==null?null:share.trim());
        if(user==null || user.trim().isEmpty() || user.length()>128)
            throw new IOException("A Windows shared-folder account is required.");
        this.user=user.trim();
        this.domain=domain==null?"":domain.trim();
        if(this.domain.length()>128)throw new IOException("Domain is too long.");
        if(password==null || password.length==0)
            throw new IOException("Enter the Windows share account password.");
        this.password=password.clone();
    }

    interface Work<T> { T run(DiskShare disk) throws IOException; }

    private synchronized <T> T withShare(Work<T> work) throws IOException {
        if(closed)throw new IOException("Laptop share session was closed.");
        SmbConfig config=SmbConfig.builder()
            .withDfsEnabled(false)
            .withSigningRequired(true)
            .withTimeout(10L,TimeUnit.SECONDS)
            .withSoTimeout(10L,TimeUnit.SECONDS)
            .build();
        try(SMBClient client=new SMBClient(config);
            Connection connection=client.connect(host);
            Session session=connection.authenticate(
                new AuthenticationContext(user,password,domain));
            DiskShare disk=(DiskShare)session.connectShare(share)) {
            return work.run(disk);
        } catch(RuntimeException error) {
            // Do not log or surface credentials, server packet dumps or secrets.
            throw new IOException("Windows SMB share unavailable or access denied.",error);
        }
    }

    List<Entry> list(String parent) throws IOException {
        String path=LaptopSharePath.directory(parent);
        return withShare(disk -> {
            List<Entry> result=new ArrayList<>();
            for(FileIdBothDirectoryInformation info:disk.list(path)) {
                String name=info.getFileName();
                if(".".equals(name) || "..".equals(name))continue;
                String child;
                try{child=LaptopSharePath.child(path,name);}
                catch(IOException invalid){continue;}
                boolean directory=(info.getFileAttributes() & 0x10L)!=0L;
                if(!directory && !DosContentSupport.isSupportedFileName(name))
                    continue;
                if(result.size()>=MAX_ENTRIES)
                    throw new IOException("Too many entries. Select a smaller Windows share folder.");
                result.add(new Entry(name,child,directory,info.getEndOfFile()));
            }
            result.sort(Comparator.comparing((Entry e)->!e.directory)
                .thenComparing(e->e.name,String.CASE_INSENSITIVE_ORDER));
            return result;
        });
    }

    DosGameModule importDos(String remotePath, DosModuleImporter importer)
        throws IOException {
        String safe=LaptopSharePath.directory(remotePath);
        String name=LaptopSharePath.fileName(safe);
        if(!DosContentSupport.isSupportedFileName(name))
            throw new IOException("Unsupported DOS content format.");
        return withShare(disk -> {
            try(File remote=disk.openFile(
                    safe,
                    EnumSet.of(AccessMask.GENERIC_READ),
                    null,
                    SMB2ShareAccess.ALL,
                    SMB2CreateDisposition.FILE_OPEN,
                    EnumSet.of(SMB2CreateOptions.FILE_NON_DIRECTORY_FILE));
                InputStream input=remote.getInputStream()) {
                return importer.importStream(input,name);
            }
        });
    }

    @Override public synchronized void close() {
        closed=true;
        Arrays.fill(password,'\\0');
    }
}
