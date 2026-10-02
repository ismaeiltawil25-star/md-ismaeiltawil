#include "SyntaxLanguages.h"

#include <QList>

namespace mdbr::render {

namespace {

QSet<QString> words(const char *spaceSeparated)
{
    QSet<QString> set;
    for (const QString &w : QString::fromLatin1(spaceSeparated).split(u' ', Qt::SkipEmptyParts))
        set.insert(w);
    return set;
}

QList<LanguageRules> buildLanguages()
{
    QList<LanguageRules> list;

    LanguageRules c;
    c.names = {"c", "h"};
    c.keywords = words("auto break case const continue default do else enum extern for goto if inline "
                       "register restrict return sizeof static struct switch typedef union volatile while");
    c.types = words("void char short int long float double signed unsigned bool size_t NULL true false");
    c.lineComments = {"//"};
    c.blockCommentOpen = "/*"; c.blockCommentClose = "*/";
    c.stringQuotes = "\"'";
    c.hashPreprocessor = true;
    list << c;

    LanguageRules cpp = c;
    cpp.names = {"cpp", "c++", "cc", "cxx", "hpp", "hxx", "h++", "qt"};
    cpp.keywords += words("alignas alignof and asm catch class concept consteval constexpr constinit "
                          "co_await co_return co_yield decltype delete explicit export friend mutable "
                          "namespace new noexcept not operator or override final private protected public "
                          "requires static_assert static_cast dynamic_cast reinterpret_cast const_cast "
                          "template this thread_local throw try typeid typename using virtual xor emit "
                          "signals slots Q_OBJECT");
    cpp.types += words("nullptr wchar_t char8_t char16_t char32_t std string vector QString QStringList "
                       "QList QHash QMap QObject QWidget qint64 qsizetype quint32");
    list << cpp;

    LanguageRules csharp = c;
    csharp.names = {"csharp", "cs", "c#"};
    csharp.hashPreprocessor = true;
    csharp.keywords = words("abstract as async await base break case catch checked class const continue "
                            "default delegate do else enum event explicit extern finally fixed for foreach "
                            "goto if implicit in interface internal is lock namespace new operator out "
                            "override params private protected public readonly record ref return sealed "
                            "sizeof stackalloc static struct switch this throw try typeof unchecked unsafe "
                            "using var virtual volatile while yield get set init");
    csharp.types = words("bool byte char decimal double float int long object sbyte short string uint "
                         "ulong ushort void null true false dynamic");
    csharp.stringQuotes = "\"'";
    list << csharp;

    LanguageRules java = c;
    java.names = {"java", "kotlin", "kt", "scala"};
    java.hashPreprocessor = false;
    java.keywords = words("abstract assert break case catch class const continue default do else enum "
                          "extends final finally for goto if implements import instanceof interface native "
                          "new package private protected public return static strictfp super switch "
                          "synchronized this throw throws transient try volatile while var val fun when "
                          "object override open data sealed record yield");
    java.types = words("boolean byte char double float int long short void String Integer Object null "
                       "true false");
    list << java;

    LanguageRules js;
    js.names = {"javascript", "js", "jsx", "mjs", "typescript", "ts", "tsx", "json5"};
    js.keywords = words("async await break case catch class const continue debugger default delete do "
                        "else export extends finally for from function if import in instanceof let new "
                        "of return static super switch this throw try typeof var void while with yield "
                        "as implements interface type enum declare readonly abstract private public "
                        "protected namespace keyof");
    js.types = words("true false null undefined NaN Infinity number string boolean any unknown never "
                     "object symbol bigint Array Object Promise Map Set console window document");
    js.lineComments = {"//"};
    js.blockCommentOpen = "/*"; js.blockCommentClose = "*/";
    js.stringQuotes = "\"'`";
    list << js;

    LanguageRules json;
    json.names = {"json", "jsonc", "geojson"};
    json.types = words("true false null");
    json.lineComments = {"//"};
    json.blockCommentOpen = "/*"; json.blockCommentClose = "*/";
    json.stringQuotes = "\"";
    list << json;

    LanguageRules python;
    python.names = {"python", "py", "python3", "pyw"};
    python.keywords = words("and as assert async await break class continue def del elif else except "
                            "finally for from global if import in is lambda nonlocal not or pass raise "
                            "return try while with yield match case");
    python.types = words("True False None self cls int float str bool list dict set tuple bytes object "
                         "print len range type isinstance");
    python.lineComments = {"#"};
    python.stringQuotes = "\"'";
    list << python;

    LanguageRules ruby;
    ruby.names = {"ruby", "rb"};
    ruby.keywords = words("alias and begin break case class def defined do else elsif end ensure for if "
                          "in module next not or redo rescue retry return self super then undef unless "
                          "until when while yield require attr_accessor");
    ruby.types = words("true false nil");
    ruby.lineComments = {"#"};
    ruby.stringQuotes = "\"'";
    list << ruby;

    LanguageRules go;
    go.names = {"go", "golang"};
    go.keywords = words("break case chan const continue default defer else fallthrough for func go goto "
                        "if import interface map package range return select struct switch type var");
    go.types = words("bool byte complex64 complex128 error float32 float64 int int8 int16 int32 int64 "
                     "rune string uint uint8 uint16 uint32 uint64 uintptr true false nil iota");
    go.lineComments = {"//"};
    go.blockCommentOpen = "/*"; go.blockCommentClose = "*/";
    go.stringQuotes = "\"'`";
    list << go;

    LanguageRules rust;
    rust.names = {"rust", "rs"};
    rust.keywords = words("as async await break const continue crate dyn else enum extern fn for if impl "
                          "in let loop match mod move mut pub ref return self Self static struct super "
                          "trait type unsafe use where while");
    rust.types = words("bool char f32 f64 i8 i16 i32 i64 i128 isize u8 u16 u32 u64 u128 usize str String "
                       "Vec Option Result Some None Ok Err Box true false");
    rust.lineComments = {"//"};
    rust.blockCommentOpen = "/*"; rust.blockCommentClose = "*/";
    rust.stringQuotes = "\"";
    list << rust;

    LanguageRules php = js;
    php.names = {"php"};
    php.keywords = words("abstract and as break case catch class clone const continue declare default do "
                         "echo else elseif empty enddeclare endfor endforeach endif endswitch endwhile "
                         "extends final finally fn for foreach function global if implements include "
                         "instanceof interface isset list match namespace new or print private protected "
                         "public require return static switch throw trait try unset use var while yield");
    php.types = words("true false null array int float string bool object mixed void self parent");
    php.lineComments = {"//", "#"};
    list << php;

    LanguageRules shell;
    shell.names = {"bash", "sh", "shell", "zsh", "console", "shellsession"};
    shell.keywords = words("if then else elif fi case esac for while until do done in function return "
                           "local export readonly declare unset shift exit source alias");
    shell.types = words("echo cd ls cat grep sed awk find mkdir rm cp mv chmod sudo git make cmake npm "
                        "pip python curl wget true false");
    shell.lineComments = {"#"};
    shell.stringQuotes = "\"'";
    list << shell;

    LanguageRules powershell;
    powershell.names = {"powershell", "ps1", "pwsh", "ps"};
    powershell.caseInsensitive = true;
    powershell.dashInWords = true;
    powershell.keywords = words("begin break catch class continue data do dynamicparam else elseif end "
                                "exit filter finally for foreach from function if in param process "
                                "return switch throw trap try until using var while");
    powershell.types = words("$true $false $null write-output write-host get-childitem set-location "
                             "new-item remove-item get-content set-content");
    powershell.lineComments = {"#"};
    powershell.blockCommentOpen = "<#"; powershell.blockCommentClose = "#>";
    powershell.stringQuotes = "\"'";
    list << powershell;

    LanguageRules sql;
    sql.names = {"sql", "mysql", "postgresql", "postgres", "sqlite", "tsql", "plsql"};
    sql.caseInsensitive = true;
    sql.keywords = words("select from where and or not insert into values update set delete create table "
                         "drop alter add column index primary key foreign references join left right inner "
                         "outer full on group by order having limit offset as distinct union all exists "
                         "in between like is null case when then else end view begin commit rollback "
                         "transaction with returning default unique check constraint asc desc");
    sql.types = words("int integer bigint smallint varchar char text date datetime timestamp boolean "
                      "decimal numeric float real double serial true false");
    sql.lineComments = {"--"};
    sql.blockCommentOpen = "/*"; sql.blockCommentClose = "*/";
    sql.stringQuotes = "'\"";
    list << sql;

    LanguageRules yaml;
    yaml.names = {"yaml", "yml"};
    yaml.types = words("true false null yes no on off ~");
    yaml.lineComments = {"#"};
    yaml.stringQuotes = "\"'";
    list << yaml;

    LanguageRules toml = yaml;
    toml.names = {"toml", "ini", "cfg", "conf", "properties"};
    toml.lineComments = {"#", ";"};
    list << toml;

    LanguageRules css;
    css.names = {"css", "scss", "less"};
    css.keywords = words("@media @import @font-face @keyframes @supports !important");
    css.types = words("px em rem vh vw auto none inherit initial");
    css.lineComments = {"//"};
    css.blockCommentOpen = "/*"; css.blockCommentClose = "*/";
    css.stringQuotes = "\"'";
    list << css;

    LanguageRules markup;
    markup.names = {"html", "xml", "xhtml", "svg", "vue", "qml-xml", "plist"};
    markup.blockCommentOpen = "<!--"; markup.blockCommentClose = "-->";
    markup.stringQuotes = "\"'";
    markup.markup = true;
    list << markup;

    LanguageRules cmake;
    cmake.names = {"cmake"};
    cmake.caseInsensitive = true;
    cmake.keywords = words("if elseif else endif foreach endforeach while endwhile function endfunction "
                           "macro endmacro return set unset option project add_executable add_library "
                           "target_link_libraries target_include_directories target_compile_definitions "
                           "find_package include add_subdirectory message install");
    cmake.types = words("on off true false public private interface required");
    cmake.lineComments = {"#"};
    cmake.stringQuotes = "\"";
    list << cmake;

    LanguageRules lua;
    lua.names = {"lua"};
    lua.keywords = words("and break do else elseif end for function goto if in local not or repeat "
                         "return then until while");
    lua.types = words("true false nil");
    lua.lineComments = {"--"};
    lua.stringQuotes = "\"'";
    list << lua;

    return list;
}

} // namespace

const LanguageRules *findLanguage(const QString &language)
{
    static const QList<LanguageRules> languages = buildLanguages();
    const QString key = language.trimmed().toLower();
    if (key.isEmpty())
        return nullptr;
    for (const LanguageRules &rules : languages)
        if (rules.names.contains(key))
            return &rules;
    return nullptr;
}

} // namespace mdbr::render
