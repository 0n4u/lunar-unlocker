package gg.lunarunlocker.lunar;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.Collections;

import static org.junit.jupiter.api.Assertions.assertEquals;

final class LunarCosmeticCatalogTest {
    @TempDir
    Path home;

    private String previousHome;

    @AfterEach
    void restoreHome() {
        if (previousHome != null) {
            System.setProperty("user.home", previousHome);
        }
        LunarCosmeticCatalog.resetForTests();
    }

    @Test
    void readsSparseHighIdsAndReloadsWhenCatalogChanges() throws Exception {
        previousHome = System.getProperty("user.home");
        System.setProperty("user.home", home.toString());
        Path catalog = home.resolve(".lunarclient/textures/assets/lunar/cosmetics.json");
        Files.createDirectories(catalog.getParent());
        Files.write(catalog,
                "[{\"id\":1},{\"id\":9674}]".getBytes(StandardCharsets.UTF_8));

        assertEquals(Arrays.asList(1, 9674), LunarCosmeticCatalog.currentIds());

        Files.write(catalog,
                "[{\"id\":1},{\"id\":7001},{\"id\":10004}]"
                        .getBytes(StandardCharsets.UTF_8));
        catalog.toFile().setLastModified(System.currentTimeMillis() + 2000L);
        assertEquals(Arrays.asList(1, 7001, 10004),
                LunarCosmeticCatalog.currentIds());
    }

    @Test
    void missingCatalogDoesNotInventIds() {
        previousHome = System.getProperty("user.home");
        System.setProperty("user.home", home.toString());
        assertEquals(Collections.emptyList(), LunarCosmeticCatalog.currentIds());
    }
}
