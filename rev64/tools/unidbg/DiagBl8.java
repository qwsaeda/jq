import com.github.unidbg.AndroidEmulator;
import com.github.unidbg.linux.android.AndroidEmulatorBuilder;
import com.github.unidbg.arm.backend.Backend;
import unicorn.ArmConst;
import java.io.*;

public class DiagBl8 {
    public static void main(String[] args) throws Exception {
        System.load("/tmp/opencode/unidbg/unicorn2-extract/natives/linux_64/libunicorn.so");
        AndroidEmulator emulator = AndroidEmulatorBuilder.for32Bit()
                .setProcessName("test")
                .addBackendFactory(new com.github.unidbg.arm.backend.Unicorn2Factory(false))
                .build();
        Backend be = emulator.getBackend();
        // 测 F7CA FC3D (0x7965a 的 BL, imm32=-0x35786) 在不同对齐地址
        int[] enc={0xF7CA,0xFC3D};
        int imm32=-0x35786;
        long[] bases={0x10000000L, 0x10000002L, 0x10000004L, 0x1219265aL};
        for (long base : bases) {
            long retAddr=base+4;
            long tgt=(retAddr+imm32)&0xFFFFFFFFL;
            long mapStart=base&~0xFFFL;
            try { be.mem_map(mapStart, 0x4000, 7); } catch(Throwable e){}
            try { be.mem_write(mapStart, new byte[0x4000]); } catch(Throwable e){}
            byte[] bl=new byte[4];
            bl[0]=(byte)(enc[0]&0xFF); bl[1]=(byte)((enc[0]>>8)&0xFF);
            bl[2]=(byte)(enc[1]&0xFF); bl[3]=(byte)((enc[1]>>8)&0xFF);
            try { be.mem_write(base, bl); } catch(Throwable e){ System.out.println("0x"+Long.toHexString(base)+": BL写入失败 "+e.getMessage()); continue; }
            long tpg=tgt&~0xFFFL;
            try { be.mem_map(tpg, 0x1000, 7); } catch(Throwable e){}
            try { be.mem_write(tpg, new byte[0x1000]); } catch(Throwable e){}
            byte[] bx={0x70,0x47};
            try { be.mem_write(tgt, bx); } catch(Throwable e){}
            try { be.mem_write(retAddr, bx); } catch(Throwable e){}
            try { be.mem_map(0x20000000, 0x1000, 7); } catch(Throwable e){}
            try { be.mem_write(0x20000000, new byte[0x1000]); } catch(Throwable e){}
            be.reg_write(10, 0x20000000L);
            long land;
            try {
                be.emu_start(base|1, 0x20000001L, 0, 10000);
                land=be.reg_read(ArmConst.UC_ARM_REG_PC).longValue();
            } catch(Throwable e) {
                land=be.reg_read(ArmConst.UC_ARM_REG_PC).longValue();
            }
            long lr=be.reg_read(ArmConst.UC_ARM_REG_LR).longValue();
            System.out.println("base=0x"+Long.toHexString(base)+" (mod4="+(base%4)+"): 结束PC=0x"+Long.toHexString(land)+" LR=0x"+Long.toHexString(lr));
            System.out.println("   正确目标=0x"+Long.toHexString(tgt)+" 返回=0x"+Long.toHexString(retAddr)+" "+(land==0x20000000L||land==retAddr?"✓":"✗ 落点异常"));
            // unmap
            try { be.mem_unmap(mapStart, 0x4000); } catch(Throwable e){}
            try { be.mem_unmap(tpg, 0x1000); } catch(Throwable e){}
            try { be.mem_unmap(0x20000000, 0x1000); } catch(Throwable e){}
        }
        emulator.close();
    }
}
