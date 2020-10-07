# Once done these will be defined:
#
#  MBEDTLS_FOUND
#  MBEDTLS_INCLUDE_DIRS
#  MBEDTLS_LIBRARIES
#

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(_MBEDTLS QUIET mbedtls)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_lib_suffix 64)
else()
	set(_lib_suffix 32)
endif()

# If we're on MacOS or Linux, please try to statically-link mbedtls.
if(STATIC_MBEDTLS AND (APPLE OR UNIX))
	set(_MBEDTLS_LIBRARIES libmbedtls.a)
	set(_MBEDCRYPTO_LIBRARIES libmbedcrypto.a)
	set(_MBEDX509_LIBRARIES libmbedx509.a)
endif()

find_path(MBEDTLS_INCLUDE_DIR
	NAMES mbedtls/ssl.h
	HINTS
		ENV mbedtlsPath${_lib_suffix}
		ENV mbedtlsPath
		ENV DepsPath${_lib_suffix}
		ENV DepsPath
		${mbedtlsPath${_lib_suffix}}
		${mbedtlsPath}
		${DepsPath${_lib_suffix}}
		${DepsPath}
		${_MBEDTLS_INCLUDE_DIRS}
	PATHS
		/usr/include /usr/local/include /opt/local/include /sw/include
	PATH_SUFFIXES
		include)

find_library(MBEDTLS_LIB
	NAMES ${_MBEDTLS_LIBRARIES} mbedtls libmbedtls
	HINTS
		ENV mbedtlsPath${_lib_suffix}
		ENV mbedtlsPath
		ENV DepsPath${_lib_suffix}
		ENV DepsPath
		${mbedtlsPath${_lib_suffix}}
		${mbedtlsPath}
		${DepsPath${_lib_suffix}}
		${DepsPath}
		${_MBEDTLS_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib /sw/lib
	PATH_SUFFIXES
		lib${_lib_suffix} lib
		libs${_lib_suffix} libs
		bin${_lib_suffix} bin
		../lib${_lib_suffix} ../lib
		../libs${_lib_suffix} ../libs
		../bin${_lib_suffix} ../bin)

find_library(MBEDCRYPTO_LIB
	NAMES ${_MBEDCRYPTO_LIBRARIES} mbedcrypto libmbedcrypto
	HINTS
		ENV mbedcryptoPath${_lib_suffix}
		ENV mbedcryptoPath
		ENV DepsPath${_lib_suffix}
		ENV DepsPath
		${mbedcryptoPath${_lib_suffix}}
		${mbedcryptoPath}
		${DepsPath${_lib_suffix}}
		${DepsPath}
		${_MBEDCRYPTO_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib /sw/lib
	PATH_SUFFIXES
		lib${_lib_suffix} lib
		libs${_lib_suffix} libs
		bin${_lib_suffix} bin
		../lib${_lib_suffix} ../lib
		../libs${_lib_suffix} ../libs
		../bin${_lib_suffix} ../bin)

find_library(MBEDX509_LIB
	NAMES ${_MBEDX509_LIBRARIES} mbedx509 libmbedx509
	HINTS
		ENV mbedx509Path${_lib_suffix}
		ENV mbedx509Path
		ENV DepsPath${_lib_suffix}
		ENV DepsPath
		${mbedx509Path${_lib_suffix}}
		${mbedx509Path}
		${DepsPath${_lib_suffix}}
		${DepsPath}
		${_MBEDX509_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib /sw/lib
	PATH_SUFFIXES
		lib${_lib_suffix} lib
		libs${_lib_suffix} libs
		bin${_lib_suffix} bin
		../lib${_lib_suffix} ../lib
		../libs${_lib_suffix} ../libs
		../bin${_lib_suffix} ../bin)

# Sometimes mbedtls is split between three libs, and sometimes it isn't.
# If it isn't, let's check if the symbols we need are all in MBEDTLS_LIB.
if(MBEDTLS_LIB AND NOT MBEDCRYPTO_LIB AND NOT MBEDX509_LIB)
	set(CMAKE_REQUIRED_LIBRARIES ${MBEDTLS_LIB})
	set(CMAKE_REQUIRED_INCLUDES ${MBEDTLS_INCLUDE_DIR})
	check_symbol_exists(mbedtls_x509_crt_init "mbedtls/x509_crt.h" MBEDTLS_INCLUDES_X509)
	check_symbol_exists(mbedtls_sha256_init "mbedtls/sha256.h" MBEDTLS_INCLUDES_CRYPTO)
	unset(CMAKE_REQUIRED_INCLUDES)
	unset(CMAKE_REQUIRED_LIBRARIES)
endif()

# If we find all three libraries, then go ahead.
if(MBEDTLS_LIB AND MBEDCRYPTO_LIB AND MBEDX509_LIB)
	set(MBEDTLS_INCLUDE_DIRS ${MBEDTLS_INCLUDE_DIR})
	set(MBEDTLS_LIBRARIES ${MBEDTLS_LIB} ${MBEDCRYPTO_LIB} ${MBEDX509_LIB})

# Otherwise, if we find MBEDTLS_LIB, and it has both CRYPTO and x509
# within the single lib (i.e. a windows build environment), then also
# feel free to go ahead.
elseif(MBEDTLS_LIB AND MBEDTLS_INCLUDES_CRYPTO AND MBEDTLS_INCLUDES_X509)
	set(MBEDTLS_INCLUDE_DIRS ${MBEDTLS_INCLUDE_DIR})
	set(MBEDTLS_LIBRARIES ${MBEDTLS_LIB})
endif()

# Now we've accounted for the 3-vs-1 library case:
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MbedTLS DEFAULT_MSG MBEDTLS_LIBRARIES MBEDTLS_INCLUDE_DIRS)
mark_as_advanced(MBEDTLS_INCLUDE_DIR MBEDTLS_LIB MBEDCRYPTO_LIB MBEDX509_LIB)
Index: lucene/src/test/org/apache/lucene/index/TestDoc.java
===================================================================
--- lucene/src/test/org/apache/lucene/index/TestDoc.java	(revision 1035374)
+++ lucene/src/test/org/apache/lucene/index/TestDoc.java	(working copy)
@@ -24,6 +24,7 @@
 import java.io.StringWriter;
 
 import java.util.LinkedList;
+import java.util.Collection;
 import java.util.List;
 import java.util.Random;
 
@@ -200,7 +201,7 @@
                                                useCompoundFile, -1, null, false, merger.hasProx(), merger.getSegmentCodecs());
       
       if (useCompoundFile) {
-        List<String> filesToDelete = merger.createCompoundFile(merged + ".cfs", info);
+        Collection<String> filesToDelete = merger.createCompoundFile(merged + ".cfs", info);
         for (final String fileToDelete : filesToDelete) 
           si1.dir.deleteFile(fileToDelete);
       }
Index: lucene/src/test/org/apache/lucene/index/TestIndexWriter.java
===================================================================
--- lucene/src/test/org/apache/lucene/index/TestIndexWriter.java	(revision 1035374)
+++ lucene/src/test/org/apache/lucene/index/TestIndexWriter.java	(working copy)
@@ -505,15 +505,15 @@
       long endDiskUsage = dir.getMaxUsedSizeInBytes();
 
       // Ending index is 50X as large as starting index; due
-      // to 2X disk usage normally we allow 100X max
+      // to 3X disk usage normally we allow 150X max
       // transient usage.  If something is wrong w/ deleter
       // and it doesn't delete intermediate segments then it
-      // will exceed this 100X:
+      // will exceed this 150X:
       // System.out.println("start " + startDiskUsage + "; mid " + midDiskUsage + ";end " + endDiskUsage);
-      assertTrue("writer used too much space while adding documents: mid=" + midDiskUsage + " start=" + startDiskUsage + " end=" + endDiskUsage,
-                 midDiskUsage < 100*startDiskUsage);
-      assertTrue("writer used too much space after close: endDiskUsage=" + endDiskUsage + " startDiskUsage=" + startDiskUsage,
-                 endDiskUsage < 100*startDiskUsage);
+      assertTrue("writer used too much space while adding documents: mid=" + midDiskUsage + " start=" + startDiskUsage + " end=" + endDiskUsage + " max=" + (startDiskUsage*150),
+                 midDiskUsage < 150*startDiskUsage);
+      assertTrue("writer used too much space after close: endDiskUsage=" + endDiskUsage + " startDiskUsage=" + startDiskUsage + " max=" + (startDiskUsage*150),
+                 endDiskUsage < 150*startDiskUsage);
       dir.close();
     }
 
@@ -539,6 +539,10 @@
       writer  = new IndexWriter(dir, newIndexWriterConfig( TEST_VERSION_CURRENT, new MockAnalyzer()).setOpenMode(OpenMode.APPEND));
       writer.optimize();
 
+      if (VERBOSE) {
+        writer.setInfoStream(System.out);
+      }
+
       // Open a reader before closing (commiting) the writer:
       IndexReader reader = IndexReader.open(dir, true);
 
@@ -558,9 +562,19 @@
       assertFalse("Reader incorrectly sees that the index is optimized", reader.isOptimized());
       reader.close();
 
-      writer  = new IndexWriter(dir, newIndexWriterConfig( TEST_VERSION_CURRENT, new MockAnalyzer()).setOpenMode(OpenMode.APPEND));
+      if (VERBOSE) {
+        System.out.println("TEST: do real optimize");
+      }
+      writer = new IndexWriter(dir, newIndexWriterConfig( TEST_VERSION_CURRENT, new MockAnalyzer()).setOpenMode(OpenMode.APPEND));
+      if (VERBOSE) {
+        writer.setInfoStream(System.out);
+      }
       writer.optimize();
       writer.close();
+
+      if (VERBOSE) {
+        System.out.println("TEST: writer closed");
+      }
       assertNoUnreferencedFiles(dir, "aborted writer after optimize");
 
       // Open a reader after aborting writer:
Index: lucene/src/test/org/apache/lucene/index/TestNRTDeletedOpenFileCount.java
===================================================================
--- lucene/src/test/org/apache/lucene/index/TestNRTDeletedOpenFileCount.java	(revision 0)
+++ lucene/src/test/org/apache/lucene/index/TestNRTDeletedOpenFileCount.java	(revision 0)
@@ -0,0 +1,190 @@
+package org.apache.lucene.index;
+
+/**
+ * Licensed to the Apache Software Foundation (ASF) under one or more
+ * contributor license agreements.  See the NOTICE file distributed with
+ * this work for additional information regarding copyright ownership.
+ * The ASF licenses this file to You under the Apache License, Version 2.0
+ * (the "License"); you may not use this file except in compliance with
+ * the License.  You may obtain a copy of the License at
+ *
+ *     http://www.apache.org/licenses/LICENSE-2.0
+ *
+ * Unless required by applicable law or agreed to in writing, software
+ * distributed under the License is distributed on an "AS IS" BASIS,
+ * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
+ * See the License for the specific language governing permissions and
+ * limitations under the License.
+ */
+
+import java.io.File;
+import java.util.Set;
+import java.util.concurrent.atomic.AtomicBoolean;
+import java.util.concurrent.atomic.AtomicInteger;
+import java.util.Random;
+
+import org.apache.lucene.analysis.MockAnalyzer;
+import org.apache.lucene.document.Document;
+import org.apache.lucene.index.codecs.CodecProvider;
+import org.apache.lucene.search.IndexSearcher;
+import org.apache.lucene.search.PhraseQuery;
+import org.apache.lucene.search.Query;
+import org.apache.lucene.search.Sort;
+import org.apache.lucene.search.SortField;
+import org.apache.lucene.search.TermQuery;
+import org.apache.lucene.store.FSDirectory;
+import org.apache.lucene.store.MockDirectoryWrapper;
+import org.apache.lucene.util.LineFileDocs;
+import org.apache.lucene.util.LuceneTestCase;
+import org.apache.lucene.util._TestUtil;
+import org.junit.Test;
+
+import static org.junit.Assert.*;
+import static org.junit.Assume.*;
+
+// TODO
+//   - update document too
+//   - delete by term/query
+//   - mix in commit, optimize, addIndexes
+//   - install a merged segment warmer that loads stored
+//     docs!!
+
+public class TestNRTDeletedOpenFileCount extends LuceneTestCase {
+
+  private final static String WIKI_LINE_FILE = "/lucene/data/enwiki-20100302-lines-1k.txt";
+
+  @Test
+  public void testDeleteOpenFileCount() throws Exception {
+
+    if (CodecProvider.getDefaultCodec().equals("SimpleText")) {
+      // no
+      CodecProvider.setDefaultCodec("Standard");
+      CodecProvider.getDefault().setDefaultFieldCodec("Standard");
+    }
+
+    final File wikiFile = new File(WIKI_LINE_FILE);
+    assumeTrue("wiki line file \"" + WIKI_LINE_FILE + "\" does not exist", wikiFile.exists());
+    
+    final LineFileDocs docs = new LineFileDocs(wikiFile);
+    final MockDirectoryWrapper dir = new MockDirectoryWrapper(random, FSDirectory.open(_TestUtil.getTempDir("nrtopenfiles")));
+    final IndexWriter writer = new IndexWriter(dir, newIndexWriterConfig(TEST_VERSION_CURRENT, new MockAnalyzer()));
+    if (VERBOSE) {
+      writer.setInfoStream(System.out);
+    }
+    MergeScheduler ms = writer.getMergeScheduler();
+    if (ms instanceof ConcurrentMergeScheduler) {
+      // try to keep max file open count down
+      ((ConcurrentMergeScheduler) ms).setMaxThreadCount(1);
+      ((ConcurrentMergeScheduler) ms).setMaxMergeCount(1);
+    }
+    LogMergePolicy lmp = (LogMergePolicy) writer.getMergePolicy();
+    if (lmp.getMergeFactor() > 5) {
+      lmp.setMergeFactor(5);
+    }
+
+    final int NUM_THREADS = 4;
+    final int RUN_TIME_SEC = 50;
+
+    final AtomicBoolean failed = new AtomicBoolean();
+    final AtomicInteger addCount = new AtomicInteger();
+    final long stopTime = System.currentTimeMillis() + RUN_TIME_SEC*1000;
+    Thread[] threads = new Thread[NUM_THREADS];
+    for(int thread=0;thread<NUM_THREADS;thread++) {
+      threads[thread] = new Thread() {
+          @Override
+            public void run() {
+            while(System.currentTimeMillis() < stopTime && !failed.get()) {
+              try {
+                Document doc = docs.nextDoc();
+                if (doc == null) {
+                  break;
+                }
+                writer.addDocument(doc);
+                addCount.getAndIncrement();
+              } catch (Exception exc) {
+                System.out.println(Thread.currentThread().getName() + ": hit exc");
+                exc.printStackTrace();
+                failed.set(true);
+                throw new RuntimeException(exc);
+              }
+            }
+          }
+        };
+      threads[thread].setDaemon(true);
+      threads[thread].start();
+    }
+
+    IndexReader r = IndexReader.open(writer);
+    boolean any = false;
+    while(System.currentTimeMillis() < stopTime && !failed.get()) {
+      Thread.sleep(_TestUtil.nextInt(random, 1, 100));
+      if (random.nextBoolean()) {
+        if (VERBOSE) {
+          System.out.println("TEST: now reopen r=" + r);
+        }
+        final IndexReader r2 = r.reopen();
+        if (r != r2) {
+          r.close();
+          r = r2;
+        }
+      } else {
+        if (VERBOSE) {
+          System.out.println("TEST: now close reader=" + r);
+        }
+        r.close();
+        writer.commit();
+        final Set<String> openDeletedFiles = dir.getOpenDeletedFiles();
+        if (openDeletedFiles.size() > 0) {
+          System.out.println("OBD files: " + openDeletedFiles);
+        }
+        any |= openDeletedFiles.size() > 0;
+        //assertEquals("open but deleted: " + openDeletedFiles, 0, openDeletedFiles.size());
+        if (VERBOSE) {
+          System.out.println("TEST: now open");
+        }
+        r = IndexReader.open(writer);
+      }
+      if (VERBOSE) {
+        System.out.println("TEST: got new reader=" + r);
+      }
+      //System.out.println("numDocs=" + r.numDocs() + " openDelFileCount=" + dir.openDeleteFileCount());
+      smokeTestReader(r);
+    }
+
+    //System.out.println("numDocs=" + r.numDocs() + " openDelFileCount=" + dir.openDeleteFileCount());
+    r.close();
+    final Set<String> openDeletedFiles = dir.getOpenDeletedFiles();
+    if (openDeletedFiles.size() > 0) {
+      System.out.println("OBD files: " + openDeletedFiles);
+    }
+    any |= openDeletedFiles.size() > 0;
+
+    assertFalse("saw non-zero open-but-deleted count", any);
+      
+    for(int thread=0;thread<NUM_THREADS;thread++) {
+      threads[thread].join();
+    }
+
+    assertEquals(addCount.get(), writer.numDocs());
+      
+    writer.close();
+    dir.close();
+    docs.close();
+  }
+
+  private void runQuery(IndexSearcher s, Query q) throws Exception {
+    s.search(q, 10);
+    s.search(q, null, 10, new Sort(new SortField("title", SortField.STRING)));
+  }
+
+  private void smokeTestReader(IndexReader r) throws Exception {
+    IndexSearcher s = new IndexSearcher(r);
+    runQuery(s, new TermQuery(new Term("body", "united")));
+    runQuery(s, new TermQuery(new Term("titleTokenized", "states")));
+    PhraseQuery pq = new PhraseQuery();
+    pq.add(new Term("body", "united"));
+    pq.add(new Term("body", "states"));
+    runQuery(s, pq);
+    s.close();
+  }
+}

Property changes on: lucene/src/test/org/apache/lucene/index/TestNRTDeletedOpenFileCount.java
___________________________________________________________________
Added: svn:eol-style
   + native

Index: lucene/src/test/org/apache/lucene/index/TestThreadedOptimize.java
===================================================================
--- lucene/src/test/org/apache/lucene/index/TestThreadedOptimize.java	(revision 1035374)
+++ lucene/src/test/org/apache/lucene/index/TestThreadedOptimize.java	(working copy)
@@ -112,11 +112,9 @@
 
       final int expectedDocCount = (int) ((1+iter)*(200+8*NUM_ITER2*(NUM_THREADS/2.0)*(1+NUM_THREADS)));
 
-      // System.out.println("TEST: now index=" + writer.segString());
+      assertEquals("index=" + writer.segString() + " numDocs=" + writer.numDocs() + " maxDoc=" + writer.maxDoc() + " config=" + writer.getConfig(), expectedDocCount, writer.numDocs());
+      assertEquals("index=" + writer.segString() + " numDocs=" + writer.numDocs() + " maxDoc=" + writer.maxDoc() + " config=" + writer.getConfig(), expectedDocCount, writer.maxDoc());
 
-      assertEquals("index=" + writer.segString() + " numDocs" + writer.numDocs() + " maxDoc=" + writer.maxDoc() + " config=" + writer.getConfig(), expectedDocCount, writer.numDocs());
-      assertEquals("index=" + writer.segString() + " numDocs" + writer.numDocs() + " maxDoc=" + writer.maxDoc() + " config=" + writer.getConfig(), expectedDocCount, writer.maxDoc());
-
       writer.close();
       writer = new IndexWriter(directory, newIndexWriterConfig(
           TEST_VERSION_CURRENT, ANALYZER).setOpenMode(
Index: lucene/src/test/org/apache/lucene/index/TestIndexWriterMergePolicy.java
===================================================================
--- lucene/src/test/org/apache/lucene/index/TestIndexWriterMergePolicy.java	(revision 1035374)
+++ lucene/src/test/org/apache/lucene/index/TestIndexWriterMergePolicy.java	(working copy)
@@ -24,7 +24,6 @@
 import org.apache.lucene.document.Field;
 import org.apache.lucene.index.IndexWriterConfig.OpenMode;
 import org.apache.lucene.store.Directory;
-import org.apache.lucene.util._TestUtil;
 
 import org.apache.lucene.util.LuceneTestCase;
 
@@ -219,7 +218,7 @@
   }
 
   private void checkInvariants(IndexWriter writer) throws IOException {
-    _TestUtil.syncConcurrentMerges(writer);
+    writer.waitForMerges();
     int maxBufferedDocs = writer.getConfig().getMaxBufferedDocs();
     int mergeFactor = ((LogMergePolicy) writer.getConfig().getMergePolicy()).getMergeFactor();
     int maxMergeDocs = ((LogMergePolicy) writer.getConfig().getMergePolicy()).getMaxMergeDocs();
@@ -261,7 +260,7 @@
         segmentCfsCount++;
       }
     }
-    assertEquals(segmentCount, segmentCfsCount);
+    assertEquals("index=" + writer.segString(), segmentCount, segmentCfsCount);
   }
 
   /*
Index: lucene/src/test/org/apache/lucene/index/TestIndexWriterReader.java
===================================================================
--- lucene/src/test/org/apache/lucene/index/TestIndexWriterReader.java	(revision 1035374)
+++ lucene/src/test/org/apache/lucene/index/TestIndexWriterReader.java	(working copy)
@@ -66,9 +66,17 @@
     boolean optimize = true;
 
     Directory dir1 = newDirectory();
-    IndexWriter writer = new IndexWriter(dir1, newIndexWriterConfig( TEST_VERSION_CURRENT, new MockAnalyzer()));
-    // test relies on no merges happening below:
-    ((LogMergePolicy) writer.getMergePolicy()).setMergeFactor(10);
+    IndexWriterConfig iwc = newIndexWriterConfig(TEST_VERSION_CURRENT, new MockAnalyzer());
+    if (iwc.getMaxBufferedDocs() < 20) {
+      iwc.setMaxBufferedDocs(20);
+    }
+    // no merging
+    if (random.nextBoolean()) {
+      iwc.setMergePolicy(NoMergePolicy.NO_COMPOUND_FILES);
+    } else {
+      iwc.setMergePolicy(NoMergePolicy.COMPOUND_FILES);
+    }
+    IndexWriter writer = new IndexWriter(dir1, iwc);
 
     // create the index
     createIndexNoClose(!optimize, "index1", writer);
@@ -129,9 +137,17 @@
     boolean optimize = false;
 
     Directory dir1 = newDirectory();
-    IndexWriter writer = new IndexWriter(dir1, newIndexWriterConfig( TEST_VERSION_CURRENT, new MockAnalyzer()));
-    // test relies on no merges happening below:
-    ((LogMergePolicy) writer.getMergePolicy()).setMergeFactor(10);
+    IndexWriterConfig iwc = newIndexWriterConfig(TEST_VERSION_CURRENT, new MockAnalyzer());
+    if (iwc.getMaxBufferedDocs() < 20) {
+      iwc.setMaxBufferedDocs(20);
+    }
+    // no merging
+    if (random.nextBoolean()) {
+      iwc.setMergePolicy(NoMergePolicy.NO_COMPOUND_FILES);
+    } else {
+      iwc.setMergePolicy(NoMergePolicy.COMPOUND_FILES);
+    }
+    IndexWriter writer = new IndexWriter(dir1, iwc);
 
     writer.setInfoStream(infoStream);
     // create the index
@@ -265,6 +281,13 @@
     
     Directory mainDir = newDirectory();
     IndexWriter mainWriter = new IndexWriter(mainDir, newIndexWriterConfig( TEST_VERSION_CURRENT, new MockAnalyzer()));
+
+    // try to keep open file count down:
+    LogMergePolicy lmp = (LogMergePolicy) mainWriter.getMergePolicy();
+    if (lmp.getMergeFactor() > 5) {
+      lmp.setMergeFactor(5);
+    }
+
     mainWriter.setInfoStream(infoStream);
     AddDirectoriesThreads addDirThreads = new AddDirectoriesThreads(numIter, mainWriter);
     addDirThreads.launchThreads(numDirs);
@@ -620,7 +643,7 @@
 
   // Stress test reopen during addIndexes
   public void testDuringAddIndexes() throws Exception {
-    Directory dir1 = newDirectory();
+    MockDirectoryWrapper dir1 = newDirectory();
     final IndexWriter writer = new IndexWriter(dir1, newIndexWriterConfig( TEST_VERSION_CURRENT, new MockAnalyzer()));
     writer.setInfoStream(infoStream);
     ((LogMergePolicy) writer.getConfig().getMergePolicy()).setMergeFactor(2);
@@ -689,10 +712,12 @@
     assertTrue(count >= lastCount);
 
     assertEquals(0, excs.size());
+    r.close();
+    assertEquals(0, dir1.getOpenDeletedFiles().size());
+
     writer.close();
 
     _TestUtil.checkIndex(dir1);
-    r.close();
     dir1.close();
   }
 
Index: lucene/src/test/org/apache/lucene/index/TestAddIndexes.java
===================================================================
--- lucene/src/test/org/apache/lucene/index/TestAddIndexes.java	(revision 1035374)
+++ lucene/src/test/org/apache/lucene/index/TestAddIndexes.java	(working copy)
@@ -834,7 +834,7 @@
     }
     c.launchThreads(-1);
 
-    Thread.sleep(500);
+    Thread.sleep(_TestUtil.nextInt(random, 10, 500));
 
     // Close w/o first stopping/joining the threads
     if (VERBOSE) {
Index: lucene/src/test/org/apache/lucene/util/LineFileDocs.java
===================================================================
--- lucene/src/test/org/apache/lucene/util/LineFileDocs.java	(revision 0)
+++ lucene/src/test/org/apache/lucene/util/LineFileDocs.java	(revision 0)
@@ -0,0 +1,131 @@
+package org.apache.lucene.util;
+
+/**
+ * Licensed to the Apache Software Foundation (ASF) under one or more
+ * contributor license agreements.  See the NOTICE file distributed with
+ * this work for additional information regarding copyright ownership.
+ * The ASF licenses this file to You under the Apache License, Version 2.0
+ * (the "License"); you may not use this file except in compliance with
+ * the License.  You may obtain a copy of the License at
+ *
+ *     http://www.apache.org/licenses/LICENSE-2.0
+ *
+ * Unless required by applicable law or agreed to in writing, software
+ * distributed under the License is distributed on an "AS IS" BASIS,
+ * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
+ * See the License for the specific language governing permissions and
+ * limitations under the License.
+ */
+
+import java.io.Closeable;
+import java.io.File;
+import java.io.IOException;
+import java.io.BufferedReader;
+import java.io.FileInputStream;
+import java.io.InputStreamReader;
+import java.io.InputStream;
+import java.io.BufferedInputStream;
+import java.util.concurrent.atomic.AtomicInteger;
+
+import org.apache.lucene.document.Document;
+import org.apache.lucene.document.Field;
+
+// Minimal port of contrib/benchmark's LneDocSource +
+// DocMaker, so tests can enum docs from a line file created
+// by contrib/benchmark's WriteLineDoc task
+public class LineFileDocs implements Closeable {
+
+  private BufferedReader reader;
+  private final File path;
+  private final static int BUFFER_SIZE = 1 << 16;     // 64K
+  private final AtomicInteger id = new AtomicInteger();
+
+  public LineFileDocs(File path) throws IOException {
+    this.path = path;
+    open();
+  }
+
+  public synchronized void close() throws IOException {
+    if (reader != null) {
+      reader.close();
+      reader = null;
+    }
+  }
+
+  private synchronized void open() throws IOException {
+    final InputStream in = new BufferedInputStream(new FileInputStream(path), BUFFER_SIZE);
+    reader = new BufferedReader(new InputStreamReader(in, "UTF-8"), BUFFER_SIZE);
+  }
+
+  public synchronized void reset() throws IOException {
+    close();
+    open();
+    id.set(0);
+  }
+
+  private final static char SEP = '\t';
+
+  private static final class DocState {
+    final Document doc;
+    final Field titleTokenized;
+    final Field title;
+    final Field body;
+    final Field id;
+    final Field date;
+
+    public DocState() {
+      doc = new Document();
+      
+      title = new Field("title", "", Field.Store.NO, Field.Index.NOT_ANALYZED_NO_NORMS);
+      doc.add(title);
+
+      titleTokenized = new Field("titleTokenized", "", Field.Store.YES, Field.Index.ANALYZED);
+      doc.add(titleTokenized);
+
+      body = new Field("body", "", Field.Store.YES, Field.Index.ANALYZED);
+      doc.add(body);
+
+      id = new Field("id", "", Field.Store.YES, Field.Index.NOT_ANALYZED_NO_NORMS);
+      doc.add(id);
+
+      date = new Field("date", "", Field.Store.YES, Field.Index.NOT_ANALYZED_NO_NORMS);
+      doc.add(date);
+    }
+  }
+
+  private final ThreadLocal<DocState> threadDocs = new ThreadLocal<DocState>();
+
+  // Document instance is re-used per-thread
+  public Document nextDoc() throws IOException {
+    String line;
+    synchronized(this) {
+      line = reader.readLine();
+      if (line == null) {       
+        return null;
+      }
+    }
+
+    DocState docState = threadDocs.get();
+    if (docState == null) {
+      docState = new DocState();
+      threadDocs.set(docState);
+    }
+
+    int spot = line.indexOf(SEP);
+    if (spot == -1) {
+      throw new RuntimeException("line: [" + line + "] is in an invalid format !");
+    }
+    int spot2 = line.indexOf(SEP, 1 + spot);
+    if (spot2 == -1) {
+      throw new RuntimeException("line: [" + line + "] is in an invalid format !");
+    }
+
+    docState.body.setValue(line.substring(1+spot2, line.length()));
+    final String title = line.substring(0, spot);
+    docState.title.setValue(title);
+    docState.titleTokenized.setValue(title);
+    docState.date.setValue(line.substring(1+spot, spot2));
+    docState.id.setValue(Integer.toString(id.getAndIncrement()));
+    return docState.doc;
+  }
+}

Property changes on: lucene/src/test/org/apache/lucene/util/LineFileDocs.java
___________________________________________________________________
Added: svn:eol-style
   + native

Index: lucene/src/java/org/apache/lucene/index/SegmentMerger.java
===================================================================
--- lucene/src/java/org/apache/lucene/index/SegmentMerger.java	(revision 1035374)
+++ lucene/src/java/org/apache/lucene/index/SegmentMerger.java	(working copy)
@@ -171,10 +171,7 @@
     }
   }
 
-  final List<String> createCompoundFile(String fileName, final SegmentInfo info)
-          throws IOException {
-    CompoundFileWriter cfsWriter = new CompoundFileWriter(directory, fileName, checkAbort);
-
+  final Collection<String> getMergedFiles(final SegmentInfo info) throws IOException {
     Set<String> fileSet = new HashSet<String>();
 
     // Basic files
@@ -203,15 +200,23 @@
       }
     }
 
+    return fileSet;
+  }
+
+  final Collection<String> createCompoundFile(String fileName, final SegmentInfo info)
+          throws IOException {
+
     // Now merge all added files
-    for (String file : fileSet) {
+    Collection<String> files = getMergedFiles(info);
+    CompoundFileWriter cfsWriter = new CompoundFileWriter(directory, fileName, checkAbort);
+    for (String file : files) {
       cfsWriter.addFile(file);
     }
     
     // Perform the merge
     cfsWriter.close();
    
-    return new ArrayList<String>(fileSet);
+    return files;
   }
 
   private void addIndexed(IndexReader reader, FieldInfos fInfos,
Index: lucene/src/java/org/apache/lucene/index/MergePolicy.java
===================================================================
--- lucene/src/java/org/apache/lucene/index/MergePolicy.java	(revision 1035374)
+++ lucene/src/java/org/apache/lucene/index/MergePolicy.java	(working copy)
@@ -69,14 +69,12 @@
     SegmentInfo info;               // used by IndexWriter
     boolean mergeDocStores;         // used by IndexWriter
     boolean optimize;               // used by IndexWriter
-    boolean increfDone;             // used by IndexWriter
     boolean registerDone;           // used by IndexWriter
     long mergeGen;                  // used by IndexWriter
     boolean isExternal;             // used by IndexWriter
     int maxNumSegmentsOptimize;     // used by IndexWriter
     SegmentReader[] readers;        // used by IndexWriter
     SegmentReader[] readersClone;   // used by IndexWriter
-    List<String> mergeFiles;            // used by IndexWriter
     public final SegmentInfos segments;
     public final boolean useCompoundFile;
     boolean aborted;
Index: lucene/src/java/org/apache/lucene/index/IndexWriter.java
===================================================================
--- lucene/src/java/org/apache/lucene/index/IndexWriter.java	(revision 1035374)
+++ lucene/src/java/org/apache/lucene/index/IndexWriter.java	(working copy)
@@ -1822,6 +1822,9 @@
     }
 
     boolean useCompoundDocStore = false;
+    if (infoStream != null) {
+      message("closeDocStores segment=" + docWriter.getDocStoreSegment());
+    }
 
     String docStoreSegment;
 
@@ -3592,8 +3595,6 @@
     if (merge.isAborted()) {
       if (infoStream != null)
         message("commitMerge: skipping merge " + merge.segString(directory) + ": it was aborted");
-
-      deleter.refresh(merge.info.name);
       return false;
     }
 
@@ -3602,13 +3603,20 @@
     commitMergedDeletes(merge, mergedReader);
     docWriter.remapDeletes(segmentInfos, merger.getDocMaps(), merger.getDelCounts(), merge, mergedDocCount);
       
+    // If the doc store we are using has been closed and
+    // is in now compound format (but wasn't when we
+    // started), then we will switch to the compound
+    // format as well:
     setMergeDocStoreIsCompoundFile(merge);
+
     merge.info.setHasProx(merger.hasProx());
 
     segmentInfos.subList(start, start + merge.segments.size()).clear();
     assert !segmentInfos.contains(merge.info);
     segmentInfos.add(start, merge.info);
 
+    closeMergeReaders(merge, false);
+
     // Must note the change to segmentInfos so any commits
     // in-flight don't lose it:
     checkpoint();
@@ -3625,11 +3633,6 @@
     return true;
   }
   
-  private synchronized void decrefMergeSegments(MergePolicy.OneMerge merge) throws IOException {
-    assert merge.increfDone;
-    merge.increfDone = false;
-  }
-
   final private void handleMergeException(Throwable t, MergePolicy.OneMerge merge) throws IOException {
 
     if (infoStream != null) {
@@ -3910,8 +3913,6 @@
       updatePendingMerges(1, false);
     }
 
-    merge.increfDone = true;
-
     merge.mergeDocStores = mergeDocStores;
 
     // Bind a new segment name here so even with
@@ -3965,14 +3966,6 @@
     // on merges to finish.
     notifyAll();
 
-    if (merge.increfDone)
-      decrefMergeSegments(merge);
-
-    if (merge.mergeFiles != null) {
-      deleter.decRef(merge.mergeFiles);
-      merge.mergeFiles = null;
-    }
-
     // It's possible we are called twice, eg if there was an
     // exception inside mergeInit
     if (merge.registerDone) {
@@ -4002,8 +3995,50 @@
         }
       }
     }
-  }        
+  }
 
+  private final synchronized void closeMergeReaders(MergePolicy.OneMerge merge, boolean suppressExceptions) throws IOException {
+    final int numSegments = merge.segments.size();
+    if (suppressExceptions) {
+      // Suppress any new exceptions so we throw the
+      // original cause
+      for (int i=0;i<numSegments;i++) {
+        if (merge.readers[i] != null) {
+          try {
+            readerPool.release(merge.readers[i], false);
+          } catch (Throwable t) {
+          }
+          merge.readers[i] = null;
+        }
+
+        if (merge.readersClone[i] != null) {
+          try {
+            merge.readersClone[i].close();
+          } catch (Throwable t) {
+          }
+          // This was a private clone and we had the
+          // only reference
+          assert merge.readersClone[i].getRefCount() == 0: "refCount should be 0 but is " + merge.readersClone[i].getRefCount();
+          merge.readersClone[i] = null;
+        }
+      }
+    } else {
+      for (int i=0;i<numSegments;i++) {
+        if (merge.readers[i] != null) {
+          readerPool.release(merge.readers[i], true);
+          merge.readers[i] = null;
+        }
+
+        if (merge.readersClone[i] != null) {
+          merge.readersClone[i].close();
+          // This was a private clone and we had the only reference
+          assert merge.readersClone[i].getRefCount() == 0;
+          merge.readersClone[i] = null;
+        }
+      }
+    }
+  }
+
   /** Does the actual (time-consuming) work of the merge,
    *  but without holding synchronized lock on IndexWriter
    *  instance */
@@ -4031,8 +4066,13 @@
 
     boolean mergeDocStores = false;
 
-    final Set<String> dss = new HashSet<String>();
-    
+    final String currentDocStoreSegment;
+    synchronized(this) {
+      currentDocStoreSegment = docWriter.getDocStoreSegment();
+    }
+
+    boolean currentDSSMerged = false;
+      
     // This is try/finally to make sure merger's readers are
     // closed:
     boolean success = false;
@@ -4040,7 +4080,6 @@
       int totDocCount = 0;
 
       for (int i = 0; i < numSegments; i++) {
-
         final SegmentInfo info = sourceSegments.info(i);
 
         // Hold onto the "live" reader; we will use this to
@@ -4059,8 +4098,8 @@
           mergeDocStores = true;
         }
         
-        if (info.getDocStoreOffset() != -1) {
-          dss.add(info.getDocStoreSegment());
+        if (info.getDocStoreOffset() != -1 && currentDocStoreSegment != null) {
+          currentDSSMerged |= currentDocStoreSegment.equals(info.getDocStoreSegment());
         }
 
         totDocCount += clone.numDocs();
@@ -4086,9 +4125,10 @@
           // readers will attempt to open an IndexInput
           // on files that have still-open IndexOutputs
           // against them:
-          if (dss.contains(docWriter.getDocStoreSegment())) {
-            if (infoStream != null)
+          if (currentDSSMerged) {
+            if (infoStream != null) {
               message("now flush at mergeMiddle");
+            }
             doFlush(true, false);
             updatePendingMerges(1, false);
           }
@@ -4099,9 +4139,7 @@
         }
 
         // Clear DSS
-        synchronized(this) {
-          merge.info.setDocStore(-1, null, false);
-        }
+        merge.info.setDocStore(-1, null, false);
       }
 
       // This is where all the work happens:
@@ -4122,26 +4160,64 @@
       //System.out.println("merger set hasProx=" + merger.hasProx() + " seg=" + merge.info.name);
       merge.info.setHasProx(merger.hasProx());
 
-      // TODO: in the non-realtime case, we may want to only
-      // keep deletes (it's costly to open entire reader
-      // when we just need deletes)
+      if (merge.useCompoundFile) {
 
-      final int termsIndexDivisor;
-      final boolean loadDocStores;
+        success = false;
+        final String compoundFileName = IndexFileNames.segmentFileName(mergedName, "", IndexFileNames.COMPOUND_FILE_EXTENSION);
 
-      synchronized(this) {
-        // If the doc store we are using has been closed and
-        // is in now compound format (but wasn't when we
-        // started), then we will switch to the compound
-        // format as well:
-        setMergeDocStoreIsCompoundFile(merge);
-        assert merge.mergeFiles == null;
-        merge.mergeFiles = merge.info.files();
-        deleter.incRef(merge.mergeFiles);
+        try {
+          if (infoStream != null) {
+            message("create compound file " + compoundFileName);
+          }
+          merger.createCompoundFile(compoundFileName, merge.info);
+          success = true;
+        } catch (IOException ioe) {
+          synchronized(this) {
+            if (merge.isAborted()) {
+              // This can happen if rollback or close(false)
+              // is called -- fall through to logic below to
+              // remove the partially created CFS:
+            } else {
+              handleMergeException(ioe, merge);
+            }
+          }
+        } catch (Throwable t) {
+          handleMergeException(t, merge);
+        } finally {
+          if (!success) {
+            if (infoStream != null) {
+              message("hit exception creating compound file during merge");
+            }
+
+            synchronized(this) {
+              deleter.deleteFile(compoundFileName);
+              deleter.deleteNewFiles(merger.getMergedFiles(merge.info));
+            }
+          }
+        }
+
+        success = false;
+
+        synchronized(this) {
+          // delete new non cfs files directly: they were never
+          // registered with IFD
+          deleter.deleteNewFiles(merger.getMergedFiles(merge.info));
+
+          if (merge.isAborted()) {
+            if (infoStream != null) {
+              message("abort merge after building CFS");
+            }
+            deleter.deleteFile(compoundFileName);
+            return 0;
+          }
+        }
+
+        merge.info.setUseCompoundFile(true);
       }
 
-      final String currentDocStoreSegment = docWriter.getDocStoreSegment();
-      
+      final int termsIndexDivisor;
+      final boolean loadDocStores;
+
       // if the merged segment warmer was not installed when
       // this merge was started, causing us to not force
       // the docStores to close, we can't warm it now
@@ -4158,118 +4234,35 @@
         loadDocStores = false;
       }
 
+      // TODO: in the non-realtime case, we may want to only
+      // keep deletes (it's costly to open entire reader
+      // when we just need deletes)
+
       final SegmentReader mergedReader = readerPool.get(merge.info, loadDocStores, BufferedIndexInput.BUFFER_SIZE, termsIndexDivisor);
       try {
         if (poolReaders && mergedSegmentWarmer != null) {
           mergedSegmentWarmer.warm(mergedReader);
         }
-        if (!commitMerge(merge, merger, mergedDocCount, mergedReader))
+
+        if (!commitMerge(merge, merger, mergedDocCount, mergedReader)) {
           // commitMerge will return false if this merge was aborted
           return 0;
+        }
       } finally {
         synchronized(this) {
           readerPool.release(mergedReader);
         }
       }
-
       success = true;
-    } finally {
-      synchronized(this) {
-        if (!success) {
-          // Suppress any new exceptions so we throw the
-          // original cause
-          for (int i=0;i<numSegments;i++) {
-            if (merge.readers[i] != null) {
-              try {
-                readerPool.release(merge.readers[i], false);
-              } catch (Throwable t) {
-              }
-            }
 
-            if (merge.readersClone[i] != null) {
-              try {
-                merge.readersClone[i].close();
-              } catch (Throwable t) {
-              }
-              // This was a private clone and we had the
-              // only reference
-              assert merge.readersClone[i].getRefCount() == 0: "refCount should be 0 but is " + merge.readersClone[i].getRefCount();
-            }
-          }
-        } else {
-          for (int i=0;i<numSegments;i++) {
-            if (merge.readers[i] != null) {
-              readerPool.release(merge.readers[i], true);
-            }
-
-            if (merge.readersClone[i] != null) {
-              merge.readersClone[i].close();
-              // This was a private clone and we had the only reference
-              assert merge.readersClone[i].getRefCount() == 0;
-            }
-          }
-        }
+    } finally {
+      // Readers are already closed in commitMerge if we didn't hit
+      // an exc:
+      if (!success) {
+        closeMergeReaders(merge, true);
       }
     }
 
-    // Must checkpoint before decrefing so any newly
-    // referenced files in the new merge.info are incref'd
-    // first:
-    synchronized(this) {
-      deleter.checkpoint(segmentInfos, false);
-    }
-    decrefMergeSegments(merge);
-
-    if (merge.useCompoundFile) {
-
-      success = false;
-      final String compoundFileName = IndexFileNames.segmentFileName(mergedName, "", IndexFileNames.COMPOUND_FILE_EXTENSION);
-
-      try {
-        merger.createCompoundFile(compoundFileName, merge.info);
-        success = true;
-      } catch (IOException ioe) {
-        synchronized(this) {
-          if (merge.isAborted()) {
-            // This can happen if rollback or close(false)
-            // is called -- fall through to logic below to
-            // remove the partially created CFS:
-            success = true;
-          } else
-            handleMergeException(ioe, merge);
-        }
-      } catch (Throwable t) {
-        handleMergeException(t, merge);
-      } finally {
-        if (!success) {
-          if (infoStream != null)
-            message("hit exception creating compound file during merge");
-          synchronized(this) {
-            deleter.deleteFile(compoundFileName);
-          }
-        }
-      }
-
-      if (merge.isAborted()) {
-        if (infoStream != null)
-          message("abort merge after building CFS");
-        deleter.deleteFile(compoundFileName);
-        return 0;
-      }
-
-      synchronized(this) {
-        if (segmentInfos.indexOf(merge.info) == -1 || merge.isAborted()) {
-          // Our segment (committed in non-compound
-          // format) got merged away while we were
-          // building the compound format.
-          deleter.deleteFile(compoundFileName);
-        } else {
-          merge.info.setUseCompoundFile(true);
-          checkpoint();
-        }
-      }
-    }
-
     return mergedDocCount;
   }
 
