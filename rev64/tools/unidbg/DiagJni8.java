import com.github.unidbg.AndroidEmulator;
import com.github.unidbg.Module;
import com.github.unidbg.Symbol;
import com.github.unidbg.linux.android.AndroidEmulatorBuilder;
import com.github.unidbg.linux.android.dvm.DvmClass;
import com.github.unidbg.linux.android.dvm.DvmObject;
import com.github.unidbg.linux.android.dvm.VM;
import com.github.unidbg.memory.Memory;
import com.github.unidbg.pointer.UnidbgPointer;
import java.io.*;
import java.util.*;

public class DiagJni8 {
    static void wr(AndroidEmulator e, long addr, long val) {
        byte[] b = new byte[4];
        for (int i=0;i<4;i++) b[i]=(byte)((val>>(8*i))&0xFF);
        e.getBackend().mem_write(addr, b);
    }
    public static void main(String[] args) throws Exception {
        System.load("/tmp/opencode/unidbg/unicorn2-extract/natives/linux_64/libunicorn.so");
        AndroidEmulator emulator = AndroidEmulatorBuilder.for32Bit()
                .setProcessName("com.cnvcs.junqi")
                .setRootDir(new File("/tmp/opencode/unidbg/rootdir"))
                .addBackendFactory(new com.github.unidbg.arm.backend.Unicorn2Factory(false))
                .build();
        for (String d : new String[]{"libc.so","libstdc++.so","libm.so","libdl.so","libGLESv1_CM.so","libGLESv2.so"}) {
            File f = new File("/tmp/opencode/unidbg/rootdir/system/lib/" + d);
            if (f.exists()) try { emulator.loadLibrary(f, false); } catch(Throwable t){}
        }
        Module so = emulator.loadLibrary(new File("/tmp/opencode/jq_apk/dec/lib/armeabi/libjunqi.so"), false);
        long base = so.base; Memory mem = emulator.getMemory();
        System.out.println("junqi base=0x" + Long.toHexString(base));
        String[] gnames = {"__stack_chk_guard","__sF","_toupper_tab_","_tolower_tab_","_ctype_","__dso_handle"};
        long[] goff = {0x15b488,0x15b4bc,0x15b51c,0x15b620,0x15b764,0x15b7d8};
        for (int i=0;i<6;i++) {
            Symbol sym = null; try { sym = so.findSymbolByName(gnames[i], true); } catch(Throwable t){}
            if (sym != null) wr(emulator, base+goff[i], sym.getAddress());
            else { int sz=(gnames[i].equals("__sF")?0x400:(gnames[i].equals("__stack_chk_guard")||gnames[i].equals("__dso_handle")?16:256));
                   UnidbgPointer bp=mem.mmap(sz,7);
                   if (gnames[i].equals("__stack_chk_guard")) bp.setInt(0,0x0A0B0C0D);
                   if (gnames[i].equals("__dso_handle")) bp.setLong(0,base);
                   if (gnames[i].equals("_toupper_tab_")||gnames[i].equals("_tolower_tab_")) for(int k=0;k<256;k++) bp.setByte(k,(byte)k);
                   if (gnames[i].equals("_ctype_")) bp.setByte(0,(byte)0x04);
                   wr(emulator, base+goff[i], bp.toUIntPeer()); }
        }
        UnidbgPointer nop = mem.mmap(4, 7); nop.setShort(0,(short)0x4770);
        List<String> lines = new ArrayList<>();
        try (BufferedReader br = new BufferedReader(new FileReader("relocs.tsv"))) { String l; while((l=br.readLine())!=null) lines.add(l); }
        for (String l : lines) {
            String[] p = l.trim().split("\\s+"); if (p.length<2) continue;
            long off=Long.parseLong(p[0],16); String name=p[1];
            if (off>=0x15b488 && off<=0x15b7d8) continue;
            if (name.equals("__aeabi_atexit")||name.equals("__cxa_atexit")||name.equals("__stack_chk_fail")) { wr(emulator, base+off, nop.toUIntPeer()|1); continue; }
            Symbol sym=null; try{ sym=so.findSymbolByName(name,true);}catch(Throwable t){}
            wr(emulator, base+off, sym!=null?sym.getAddress():(nop.toUIntPeer()|1));
        }
        long dataEnd=(base+0x162e88+0xfff)&~0xfffL;
        long mapEnd=base+0x200000;
        if (mapEnd>dataEnd){ try{ UnidbgPointer ex=mem.mmap((int)(mapEnd-dataEnd),7); for(int i=0;i<(int)(mapEnd-dataEnd);i++) ex.setByte(i,(byte)0);}catch(Throwable t){} }
        byte[] bx = new byte[2]; bx[0]=(byte)0x70; bx[1]=(byte)0x47;
        emulator.getBackend().mem_write(base+0x11af58, bx);
        long[] inits={0x3dae5,0x6ef21,0xafbd9,0xafc85,0xaffd5,0xb01fd,0xb06f1,0xb1d69,0xb222d,0x105e8d,0x10a58d,0x11c10d};
        for(long a: inits){ try{ so.callFunction(emulator,a);}catch(Throwable t){} }
        System.out.println("init_array done");

        // === 补环境：创建 DalvikVM + 解析 App 类 + 构造 Activity ===
        VM vm = emulator.createDalvikVM();
        DvmClass app = vm.resolveClass("com/cnvcs/App");
        System.out.println("App class resolved: " + app.getClassName());
        DvmClass activityClass = vm.resolveClass("android/app/Activity");
        DvmObject<?> activity = activityClass.newObject("FakeActivity");
        System.out.println("Activity obj = " + activity);

        // OnCreate(Activity) → GLView。标准 unidbg JNI 调用（自动设 JNIEnv）。
        final long[] lastpc={0}; final long[] vecfix={0}; final long[] hist=new long[40]; final int[] hi={0};
        final long b=base;
        // 安全 scratch（0x60000000，不冲突 288MB mmap @0x3743f000-0x4947d000）
        final long[] scratch={0};
        try {
            long sp2=0x60000000L;
            emulator.getBackend().mem_map(sp2, 0x10000, 7);
            scratch[0]=sp2;
            emulator.getBackend().mem_write(sp2, new byte[0x10000]);
            System.out.println("scratch 0x60000000+64KB 映射");
        } catch(Throwable t){ System.out.println("scratch err: "+t.getMessage()); }
        // hook @0x7965a: 强制 BL → 0x43ed8
        emulator.getBackend().hook_add_new(new com.github.unidbg.arm.backend.CodeHook() {
            public void hook(com.github.unidbg.arm.backend.Backend bd, long address, int size, Object user) {
                if (address==(b+0x7965a)) {
                    System.out.println("  [BLfix] BL @0x7965a → 强制 0x43ed8");
                    bd.reg_write(unicorn.ArmConst.UC_ARM_REG_PC, b+0x43ed8);
                    bd.reg_write(unicorn.ArmConst.UC_ARM_REG_LR, b+0x7965e);
                }
            }
            public void onAttach(com.github.unidbg.arm.backend.UnHook h) {}
            public void detach() {}
        }, b+0x7965a, b+0x7965a+2, null);
        // hook @0x43ed8: 修 vector 源 begin/end 垃圾 → 空 vector
        emulator.getBackend().hook_add_new(new com.github.unidbg.arm.backend.CodeHook() {
            public void hook(com.github.unidbg.arm.backend.Backend bd, long address, int size, Object user) {
                lastpc[0]=address; hist[hi[0]%40]=address; hi[0]++;
                if (address==(b+0x43ed8)) {
                    long r0=bd.reg_read(unicorn.ArmConst.UC_ARM_REG_R0).longValue();
                    long r1=bd.reg_read(unicorn.ArmConst.UC_ARM_REG_R1).longValue();
                    long r2=bd.reg_read(unicorn.ArmConst.UC_ARM_REG_R2).longValue();
                    vecfix[0]++;
                    long sp=bd.reg_read(unicorn.ArmConst.UC_ARM_REG_SP).longValue();
                    if (vecfix[0]<=3) System.out.println("  [vec] 0x43ed8 #"+vecfix[0]+": r0=0x"+Long.toHexString(r0)+" r1=0x"+Long.toHexString(r1)+" r2=0x"+Long.toHexString(r2)+" sp=0x"+Long.toHexString(sp));
                    // r1/r2 垃圾（r2<r1 无效区间 或 超范围）→ scratch 空区
                    if (scratch[0]!=0 && (r2<r1 || r1<0x10000000L || r2>0x70000000L)) {
                        bd.reg_write(unicorn.ArmConst.UC_ARM_REG_R1, scratch[0]);
                        bd.reg_write(unicorn.ArmConst.UC_ARM_REG_R2, scratch[0]);
                        System.out.println("    → 修 r1/r2 为空 vector");
                    }
                }
            }
            public void onAttach(com.github.unidbg.arm.backend.UnHook h) {}
            public void detach() {}
        }, b+0x43ed8, b+0x43f04, null);
        try {
            // 确认 0x43ed8 / 0x7965a 字节可读
            byte[] c43=emulator.getBackend().mem_read(b+0x43ed8,4);
            byte[] c79=emulator.getBackend().mem_read(b+0x7965a,4);
            System.out.println("  0x43ed8 字节="+String.format("%02x%02x%02x%02x",c43[0],c43[1],c43[2],c43[3])+" 0x7965a 字节="+String.format("%02x%02x%02x%02x",c79[0],c79[1],c79[2],c79[3]));
            DvmObject<?> glview = app.callStaticJniMethodObject(emulator, "OnCreate", activity);
            System.out.println("OnCreate -> " + glview + (glview!=null?" (class="+glview.getObjectType()+")":""));
        } catch(Throwable t) {
            System.out.println("OnCreate THREW " + t.getClass().getSimpleName() + ": " + t.getMessage() + "  vecfix="+vecfix[0]+"  最后pc=0x"+Long.toHexString(lastpc[0]));
            System.out.print("  vec区最近: ");
            for (int i=0;i<Math.min(hi[0],40);i++) System.out.print(Long.toHexString(hist[i]-b)+" ");
            System.out.println();
        }
        emulator.close();
    }
}
