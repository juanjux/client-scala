import sys.process._

name := "bblfsh-client"
organization := "org.bblfsh"
version := "1.5.3"

scalaVersion := "2.11.11"
val libuastVersion = "v1.6.0"


mainClass in Compile := Some("org.bblfsh.client.cli.ScalaClientCLI")

target in assembly := file("build")

PB.targets in Compile := Seq(
  scalapb.gen() -> (sourceManaged in Compile).value
)
PB.protoSources in Compile := Seq(file("src/main/proto"))

libraryDependencies += "com.trueaccord.scalapb" %% "scalapb-runtime" % com.trueaccord.scalapb.compiler.Version.scalapbVersion % "protobuf"
libraryDependencies += "commons-io" % "commons-io" % "2.5"
libraryDependencies ++= Seq(
  "org.scalatest" %% "scalatest" % "3.0.1" % "test",

  "io.grpc" % "grpc-netty" % com.trueaccord.scalapb.compiler.Version.grpcJavaVersion,
  "com.trueaccord.scalapb" %% "scalapb-runtime-grpc" % com.trueaccord.scalapb.compiler.Version.scalapbVersion,

  "org.rogach" %% "scallop" % "3.0.3"
)

assemblyMergeStrategy in assembly := {
  case "META-INF/io.netty.versions.properties" => MergeStrategy.last
  case x =>
    val oldStrategy = (assemblyMergeStrategy in assembly).value
    oldStrategy(x)
}

test in assembly := {}

sonatypeProfileName := "org.bblfsh"

// pom settings for sonatype
homepage := Some(url("https://github.com/bblfsh/client-scala"))
scmInfo := Some(ScmInfo(url("https://github.com/bblfsh/client-scala"),
                            "git@github.com:bblfsh/client-scala.git"))
developers += Developer("juanjux",
                        "Juanjo Álvarez",
                        "juanjo@sourced.tech",
                        url("https://github.com/juanjux"))
licenses += ("Apache-2.0", url("http://www.apache.org/licenses/LICENSE-2.0"))
pomIncludeRepository := (_ => false)

crossPaths := false
publishMavenStyle := true
exportJars := true

val SONATYPE_USERNAME = scala.util.Properties.envOrElse("SONATYPE_USERNAME", "NOT_SET")
val SONATYPE_PASSWORD = scala.util.Properties.envOrElse("SONATYPE_PASSWORD", "NOT_SET")
credentials += Credentials(
  "Sonatype Nexus Repository Manager",
  "oss.sonatype.org",
  SONATYPE_USERNAME,
  SONATYPE_PASSWORD)

val SONATYPE_PASSPHRASE = scala.util.Properties.envOrElse("SONATYPE_PASSPHRASE", "not set")

useGpg := false
pgpSecretRing := baseDirectory.value / "project" / ".gnupg" / "secring.gpg"
pgpPublicRing := baseDirectory.value / "project" / ".gnupg" / "pubring.gpg"
pgpPassphrase := Some(SONATYPE_PASSPHRASE.toArray)

isSnapshot := version.value endsWith "SNAPSHOT"

publishTo := {
  val nexus = "https://oss.sonatype.org/"
  if (isSnapshot.value)
    Some("snapshots" at nexus + "content/repositories/snapshots")
  else
    Some("releases" at nexus + "service/local/staging/deploy/maven2")
}

val getLibuast = TaskKey[Unit]("getLibuast", "Retrieve libuast")
getLibuast := {
    // XXX
    //import sys.process._

    //println("Downloading libuast...")

    //f"curl -SL https://github.com/bblfsh/libuast/releases/download/$libuastVersion%s/libuast-$libuastVersion%s.tar.gz -o libuast.tar.gz" #&&
    //"tar zxf libuast.tar.gz" #&&
    //f"mv libuast-$libuastVersion%s libuast" #&&
    //"rm -rf src/libuast-native" #&&
    //"mv libuast/src/ src/libuast-native" #&&
    //"rm -rf libuast" #&&
    //"rm libuast.tar.gz" !
}

val compileLibuast = TaskKey[Unit]("compileLibuast", "Compile libUAST")
compileLibuast := {
    import sys.process._

    println("Compiling libuast bindings...")

    "mkdir ./lib" !

    val sourceFiles = "src/main/scala/org/bblfsh/client/libuast/org_bblfsh_client_libuast_Libuast.cc " +
        "src/main/scala/org/bblfsh/client/libuast/jni_utils.cc " +
        "src/main/scala/org/bblfsh/client/libuast/nodeiface.cc " +
        "src/main/scala/org/bblfsh/client/libuast/objtrack.cc " +
        "src/libuast-native/uast.cc " +
        "src/libuast-native/roles.c "

    compileLinux(sourceFiles)
    compileMacOS(sourceFiles)
}

def compileLinux(sourceFiles: String) = {
  import sys.process._

  var javaHome = System.getenv("JAVA_HOME")
  if (javaHome == null) {
    javaHome = "/usr/lib/jvm/java-8-openjdk-amd64"
  }
  val xml2Conf = "xml2-config --cflags --libs" !!

  val cmd:String = "g++ -shared -Wall -fPIC -O2 " +
    "-I/usr/include " +
    "-I" + javaHome + "/include/ " +
    "-I" + javaHome + "/include/linux " +
    "-Isrc/libuast-native/  " +
    "-o lib/libscalauast.so " + 
    sourceFiles +
    xml2Conf + " "

  checkedProcess(cmd, "macOS build")
}

def compileMacOS(sourceFiles: String): Unit = {
  val osxHome = System.getenv("OSXCROSS_PATH")

  if (osxHome == null || osxHome.isEmpty) {
    println("OSXCROSS_PATH variable not defined, not cross-compiling for macOS")
    return
  }

  val cmd = osxHome + "/bin/o64-clang -shared -Wall -fPIC -O2 -lxml2 " +
      "-I" + osxHome + "/SDK/MacOSX10.11.sdk/usr/include/libxml2/ " +
      "-I" + osxHome + "/SDK/src/libuast-native/roles.c " +
      "-I" + osxHome + "/SDK/MacOSX10.11.sdk/usr/include/ " +
      "-I/usr/lib/jvm/java-8-openjdk-amd64/include " +
      "-I/usr/lib/jvm/java-8-openjdk-amd64/include/linux " +
      "-Isrc/libuast-native/ -o lib/libscalauast.dylib " +
      sourceFiles

  checkedProcess(cmd, "macOS build")
} 

def checkedProcess(cmd: String, name: String) {
  import sys.process._

  println(cmd)
  val out = cmd !

  if (out != 0) {
    throw new IllegalStateException(name + " failed (see previous messages)")
  }
}

mappings in (Compile, packageBin) += {
    (baseDirectory.value / "lib" / "libscalauast.so") -> "lib/libscalauast.so"
}
mappings in (Compile, packageBin) += {
    (baseDirectory.value / "lib" / "libscalauast.dylib") -> "lib/libscalauast.dylib"
}

mainClass := Def.sequential(getLibuast, compileLibuast, (mainClass in Compile)).value

