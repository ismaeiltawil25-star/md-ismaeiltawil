# Extended Syntax Test

Open this file in the reader to check every extension visually.
Jump to [the math section](#math) or [the alerts](#alerts).

## Inline extensions

- Highlight: this is ==important text== in a sentence.
- Superscript: E = mc^2^ and 2^10^ = 1024.
- Emoji: shipped :rocket: :tada: :white_check_mark: :warning: :+1:
- Strikethrough: ~~old~~ new.
- Footnotes: Markdown was created in 2004[^md], and GitHub added footnotes later[^gh].

## Math

Inline: the area is $\pi r^2$ and $a^2 + b^2 = c^2$.

$$
\sum_{i=1}^{n} i = \frac{n(n+1)}{2}
$$

$$
\forall x \in \mathbb{R}: \sqrt{x^2} = |x|
$$

## Alerts

> [!NOTE]
> Useful information that users should know.

> [!TIP]
> Helpful advice for doing things better.

> [!IMPORTANT]
> Key information users need to know.

> [!WARNING]
> Urgent info that needs immediate attention.

> [!CAUTION]
> Advises about risks or negative outcomes.

## Syntax highlighting

```cpp
#include <vector>
// Sum all values
int sum(const std::vector<int> &values) {
    int total = 0;
    for (int v : values) total += v;
    return total; /* done */
}
```

```python
def greet(name: str) -> str:
    """Return a greeting."""
    return f"Hello, {name}!"  # comment
```

```sql
SELECT name, COUNT(*) AS total FROM users WHERE active = 1 GROUP BY name;
```

## Wiki links

See [[comprehensive-visual-test]] for the full base test.

[^md]: John Gruber and Aaron Swartz, 2004.
[^gh]: GitHub announced footnote support in 2021.
