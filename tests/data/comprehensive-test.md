---
title: "اختبار شامل لقدرات Markdown"
author: "MD Backpack Reader Test Suite"
date: 2026-05-24
language: ar
tags: [test, markdown, comprehensive, arabic, rtl]
---

# MD Backpack Reader — اختبار شامل

> هذا المستخدَم يُختبر به كل إمكانيات البرنامج.  
> Written to validate parsing, rendering, RTL, raw HTML, and edge cases.

---

## 1. العناوين / Headings

# Heading 1 — العنوان الرئيسي
## Heading 2 — عنوان فرعي
### Heading 3 — مستوى ثالث
#### Heading 4 — مستوى رابع
##### Heading 5 — مستوى خامس
###### Heading 6 — مستوى سادس

---

## 2. النصوص المنسقة / Text Formatting

**نص عريض** — **bold text**

*نص مائل* — *italic text*

***نص عريض ومائل*** — ***bold italic***

~~نص مشطوب~~ — ~~strikethrough~~

`كود برمجي داخل السطر` — `inline code`

---

## 3. الروابط / Links

- [رابط خارجي](https://example.com)
- [رابط مع عنوان](https://example.com "Example Title")
- <https://raw.githubusercontent.com>
- البريد: <test@example.com>

---

## 4. الصور / Images

![Placeholder](https://via.placeholder.com/150 "صورة اختبار")

---

## 5. القوائم غير المرتبة / Unordered Lists

- عنصر أول
- عنصر ثانٍ
  - عنصر فرعي
  - عنصر فرعي آخر
    - عنصر من المستوى الثالث
- عنصر ثالث

---

## 6. القوائم المرتبة / Ordered Lists

1. الخطوة الأولى
2. الخطوة الثانية
   1. خطوة فرعية أولى
   2. خطوة فرعية ثانية
3. الخطوة الثالثة

---

## 7. قوائم المهام / Task Lists

- [x] مهمة مكتملة
- [ ] مهمة غير مكتملة
- [x] مهمة أخرى مكتملة
- [ ] مهمة معلقة
- [ ] مهمة ذات أولوية

---

## 8. الجداول / Tables

| الميزة          | الحالة     | الملاحظات            |
|-----------------|------------|----------------------|
| العناوين        | ✅         | تعمل بكفاءة          |
| الجداول         | ✅         | تدعم المحاذاة       |
| القوائم         | ✅         | عادية ومهام         |
| كود برمجي       | ✅         | مع إبراز الصيغة     |
| الاقتباسات      | ✅         | تدعم التداخل        |

جدول بمحاذاة مختلفة:

| محاذاة يسار | محاذاة وسط | محاذاة يمين |
|:-------------|:-----------:|-------------:|
| نص           |    نص       |          نص |
| 123          |    456      |          789 |

---

## 9. الكود البرمجي / Code Blocks

### C++
```cpp
#include <iostream>
#include <string>

int main() {
    std::string name = "MD Backpack Reader";
    std::cout << "Hello, " << name << "!" << std::endl;

    for (int i = 0; i < 10; ++i) {
        if (i % 2 == 0) {
            std::cout << i << " is even" << std::endl;
        }
    }

    return 0;
}
```

### JavaScript
```javascript
/**
 * Calculate fibonacci sequence
 * @param {number} n
 * @returns {number}
 */
function fibonacci(n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

const result = fibonacci(10);
console.log(`Fibonacci(10) = ${result}`);
```

### Python
```python
from typing import List, Optional


class MarkdownParser:
    """A simple markdown parser implementation."""

    def __init__(self, text: str):
        self.text = text
        self.lines = text.split('\n')
        self.pos = 0

    def parse_heading(self) -> Optional[str]:
        line = self.lines[self.pos]
        if line.startswith('#'):
            level = len(line.split(' ')[0])
            return f'<h{level}>{line[level + 1:]}</h{level}>'
        return None

    def parse_all(self) -> str:
        html_parts = []
        while self.pos < len(self.lines):
            result = self.parse_heading()
            if result:
                html_parts.append(result)
            self.pos += 1
        return '\n'.join(html_parts)
```

### PowerShell
```powershell
function Get-MarkdownStats {
    param(
        [Parameter(Mandatory)]
        [string]$FilePath
    )

    $content = Get-Content -Path $FilePath -Raw
    $lines = $content -split "`n"

    $stats = [PSCustomObject]@{
        FileName   = Split-Path $FilePath -Leaf
        LineCount  = $lines.Count
        CharCount  = $content.Length
        Headings   = ($lines | Where-Object { $_ -match '^#{1,6}\s' }).Count
        CodeBlocks = ($lines | Where-Object { $_ -match '^```' }).Count / 2
    }

    return $stats
}

$stats = Get-MarkdownStats -FilePath "test.md"
Write-Host "Headings found: $($stats.Headings)"
```

### SQL
```sql
-- Retrieve markdown documents with statistics
WITH doc_stats AS (
    SELECT
        id,
        title,
        LENGTH(content) AS char_count,
        (LENGTH(content) - LENGTH(REPLACE(content, '\n', ''))) + 1 AS line_count
    FROM documents
    WHERE content_type = 'markdown'
)
SELECT
    id,
    title,
    char_count,
    line_count,
    CASE
        WHEN char_count > 10000 THEN 'large'
        WHEN char_count > 1000  THEN 'medium'
        ELSE 'small'
    END AS size_category
FROM doc_stats
ORDER BY char_count DESC;
```

### JSON
```json
{
  "name": "MD Backpack Reader",
  "version": "0.2.0",
  "features": {
    "headings": true,
    "tables": true,
    "codeBlocks": true,
    "taskLists": true,
    "rtlSupport": true,
    "yamlFrontMatter": true,
    "rawHtml": true
  },
  "languages": [
    "C++",
    "JavaScript",
    "Python",
    "PowerShell",
    "SQL"
  ],
  "maxFileSize": 10485760
}
```

### YAML
```yaml
# Test configuration
app:
  name: MD Backpack Reader
  version: 0.2.0
  debug: false

renderer:
  flags:
    - collapse_whitespace
    - tables
    - task_lists
  max_input_size: 10485760

rtl:
  enabled: true
  scripts:
    - arabic
    - persian
    - urdu
    - hebrew

logging:
  level: info
  format: "[{timestamp}] [{level}] {message}"
```

---

## 10. الاقتباسات / Blockquotes

> اقتباس بسيط.

> اقتباس متعدد الأسطر  
> يستمر في السطر التالي  
> وينتهي هنا.

> **اقتباس بداخله تنسيق**  
> يحتوي على `كود برمجي` ورابط: [رابط](https://example.com)

> اقتباس من المستوى الأول
>
> > اقتباس من المستوى الثاني  
> > > اقتباس من المستوى الثالث

---

## 11. وسوم HTML الخام / Raw HTML Tags

اختبر اختصار لوحة المفاتيح: <kbd>Ctrl</kbd> + <kbd>O</kbd> لفتح ملف، و<kbd>Ctrl</kbd> + <kbd>Q</kbd> للخروج.

هذا نص <mark>مظلل بالأصفر</mark> للفت الانتباه.

<details>
<summary>اضغط لعرض التفاصيل</summary>

**محتوى مخفي:** هذا النص يظهر فقط عند فتح التفاصيل في المتصفحات العادية.  
في هذا البرنامج، يظهر المحتوى مباشرة بدون تفاعل.

```python
print("Hidden code block inside details")
```
</details>

<div style="border:2px solid #4A90D9; padding:12px; border-radius:8px; background:#F0F8FF;">

**مربع مخصص:** هذا النص داخل عنصر `<div>` تم تحويله إلى جدول لعرضه بشكل صحيح مع الخلفية والحدود.

</div>

الصيغ الكيميائية: H<sub>2</sub>O  
المعادلات الفيزيائية: E=mc<sup>2</sup>

---

## 12. النصوص من اليمين إلى اليسار / RTL Support

### فقرة عربية (Arabic Paragraph)

هذا النص باللغة العربية. يجب أن يظهر من اليمين إلى اليسار مع محاذاة صحيحة.  
مرحباً بكم في برنامج MD Backpack Reader الذي يدعم اللغات التي تكتب من اليمين إلى اليسار.

### قائمة عربية (Arabic List)

- العنصر الأول في القائمة العربية
- العنصر الثاني يحتوي على كود `console.log("Hello")`
- العنصر الثالث: ١٢٣ + ٤٥٦ = ٥٧٩

### فقرة فارسية (Persian/Farsi Paragraph)

این یک متن فارسی برای تست پشتیبانی از زبان‌های راست‌به‌چپ است.  
برنامه MD Backpack Reader از نمایش صحیح متون فارسی پشتیبانی می‌کند.

### نص مختلط (Mixed Arabic & English)

هذا النص يحتوي على English words embedded within Arabic text.  
رقم الهاتف: +1 (555) 123-4567 والبريد الإلكتروني: test@example.com  
الموقع: 123 Main Street, New York, NY 10001

### كود برمجي عربي (Arabic in Code Block — يجب ألا يتأثر بـ RTL)

```text
نص عربي داخل كتلة كود برمجي
يجب أن يبقى كما هو بدون إضافة dir="rtl"
لأن الكود البرمجي لا يجب أن يتغير اتجاهه
```

---

## 13. فواصل أفقية / Horizontal Rules

---

***

___

---

## 14. قوائم التعريف / Definition Lists (إن كانت مدعومة)

MD Backpack Reader
: قارئ Markdown خفيف وسريع مبني على Qt 6

Technical Debt
: الديون التقنية التي تراكمت نتيجة قرارات برمجية سابقة

Modular Architecture
: بنية معيارية تسهل الصيانة والتطوير المستقبلي

---

## 15. حالات الحافة / Edge Cases

### سطر فارغ فقط

### ملف بدون محتوى

### عدد كبير من الرموز الخاصة

!@#$%^&*()_+-=[]{}|;':",./<>?~\`'"¡™£¢∞§¶•ªº–≠

### هروب الرموز (Escaping)

\*هذا ليس مائلاً\*  
\# هذا ليس عنواناً  
\[هذا ليس رابطاً\](https://example.com)

### مسافات متعددة

كلمة     كلمة     كلمة

### نص طويل جداً

هذا النص الطويل جداً يستخدم لاختبار قدرة البرنامج على عرض الأسطر الطويلة دون كسر أو اقتصاص غير متوقع. يجب أن يستمر النص في السطر نفسه حتى نهاية السطر المرئي ثم ينتقل إلى السطر التالي بشكل طبيعي وسلس ودون أي مشاكل في العرض أو التنسيق. هذا اختبار مهم جداً لضمان عمل البرنامج بشكل صحيح مع النصوص العربية والانجليزية الطويلة.

---

## 16. تنسيقات متداخلة / Nested Formatting

> **🔥 تحذير مهم:** هذا النص يحتوي على:
> - **نص عريض** داخل اقتباس
> - `كود برمجي` داخل اقتباس
> - [رابط](https://example.com) داخل اقتباس
> - > اقتباس داخل اقتباس

1. **عنصر مهم** مع `كود` ورابط
2. *عنصر مائل* مع ~~نص مشطوب~~
3. `const x = 42;` — كود داخل قائمة مرتبة

---

## 17. إسنادات المراجع / Reference-Style Links

[MD Backpack Reader][1]
[Qt Framework][2]
[CMake Build System][3]

[1]: https://github.com/example/md-backpack-reader
[2]: https://www.qt.io
[3]: https://cmake.org

---

## 18. الحواشي السفلية / Footnotes

هذا النص يحتوي على حاشية[^1] وأخرى[^2].

[^1]: هذه هي الحاشية الأولى مع شرح تفصيلي.
[^2]: هذه هي الحاشية الثانية مع معلومات إضافية.

---

## 19. جداول معقدة / Complex Tables

| الاسم    | نوع البيانات | القيمة الافتراضية | الإلزامي | الوصف                              |
|----------|---------------|-------------------|----------|------------------------------------|
| `title`  | `string`      | `""`              | نعم      | عنوان المستند                      |
| `author` | `string`      | `"Unknown"`       | لا       | اسم المؤلف                         |
| `date`   | `date`        | اليوم             | لا       | تاريخ الإنشاء                      |
| `tags`   | `string[]`    | `[]`              | لا       | قائمة الوسوم                        |
| `draft`  | `boolean`     | `false`           | لا       | هل هو مسودة؟                        |

---

## 20. خلاصة / Summary

> ✅ **تم اجتياز الاختبار بنجاح**  
> إذا رأيت هذا النص معناه أن البرنامج يعمل بشكل صحيح  
> ويدعم جميع الميزات المذكورة أعلاه.

---

*انتهى ملف الاختبار — Test file end*
