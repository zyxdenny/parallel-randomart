#import "@preview/touying:0.5.3": *
#import themes.simple: *

#show: simple-theme.with(aspect-ratio: "4-3")
#show figure.caption: it => [
  #it.body
]


= Parallel Random-Art Hash Visualization

Yuxuan Zheng

#figure(
  grid(
    columns: (1fr, 1fr, 1fr),
    gutter: 3pt,
    image("img-sample/sample1.png"),
    image("img-sample/sample2.png"),
    image("img-sample/sample3.png"),
  ),
)



== Problem Definition

_Random Art_ is a smart solution based on context-free grammar that makes hash comparison easier for human beings.

#uncover("2-")[
  #text(18pt)[
    Example:
    "`f39dc904f9ccb1be0669481c`" (hash string, serves as seed) $#sym.arrow$
    #box(
      height: 50pt,
      image("img-sample/sample1.png")
    )
  ]
]

#uncover("3-")[
  A grammar generates random _expression trees_. A random art is rendered by passing the coordinate $(x, y)$ 
  of each pixel into the expression tree of each colour channel (RGB).
]

#only("4")[
  #figure(
    image("tree.png", width: 35%),
  )
]

#only("5-")[
  The tree has to be complex enough to achieve the pre-image resistant property of a hash function.
  However, the computation work grows exponentially with respect to the depth of tree. We need a way
  to make this faster.
]

== Literature Survey

- *Hash Visualization: a New Technique to improve Real-World Security*
  - Propose _random art_ as the hash visualisation method
- *OpenMP manual*
  - Use `task` for recursive tree traversal
- *A Practical Tree Contraction Algorithm for Parallel Skeletons on Trees of Unbounded Degree* 
  - Proposes _Rake-Shunt_ tree contraction algorithm
  - Compress a tree iteratively by removing leaves and merging nodes, e.g, combining operations like $f(g(x))$ into $(f circle.stroked.small g)x$
  - However, merging nodes involves creating partial functions, which is easy in a functional language but tricky in C

== Experimental Setup

Two types of parallelisation:
- Pixel-rendering level parallelism
  - Trees of depth 5, 8, 10, and 15
  - Thread number 1, 2, 4, 8, 16, 32
- Expression-tree-evaluation level parallelism
  - Trees of depth 5, 8, 10, and 15
  - Thread number 1, 2, 4, 8, 16, 32
  - Three strategies to assign tasks:
    - Assign tasks in every recursion if `depth < THRESHOLD`
    - Assign tasks only if `depth == THRESHOLD`
    - Assign tasks only if `depth == THRESHOLD`, and in each recursion, instead of doing `depth++`, do `depth = (depth + 1) % (THRESHOLD + 1)`

== Results & Analysis

#text(18pt)[
Pixel-rendering level parallelism: Embarassingly parallel
#figure(
  image("plot-pdata.png", width: 35%),
)

Tree-evaluation level parallelism: Worse performance as the number of threads increases
#figure(
  grid(
    columns: (1fr, 1fr, 1fr),
    gutter: 3pt,
    image("plot-rdata1.png"),
    image("plot-rdata2.png"),
    image("plot-rdata3.png"),
  ),
  caption: [Plots of strategy 1, 2, 3, respectively, with `THRESHOLD = 4`],
)
]

== Result & Analysis

#text(18pt)[
Experiment: Does the average arity of the functions in the grammar affect the speedup for tree-evaluation level parallelism?

#grid(
  columns: (1fr, 1fr),
  gutter: 3pt,
  [ $ P &#sym.arrow (C, C, C)  \
    A &#sym.arrow  X^(0.333) | Y^(0.333) | "RAND"()^(0.333) \
    B &#sym.arrow "EIGHT_SUM"(A, A, A, A, A, A, A, A)^1 \
    C &#sym.arrow "EIGHT_SUM"(A, A, D, E, A, B, D, E)^1 \
    D &#sym.arrow "ADD"(B, A)^(0.25)  | "MIX"(C, D, E)^(0.75)  \
    E &#sym.arrow "ADD"(A, A)^(0.5)  | "MULT"(B, B)^(0.25)  | C^(0.25)  $
  ],
  [
    #figure(
      caption: "Relationship between average arity and speedup",
      table(
        columns: (1fr, 1fr),
        inset: 8pt,
        align: horizon,
        table.header(
          [*Average arity*], [*Speedup*],
        ),
        [1.2], [0.42],
        [1.8], [0.48],
        [2.2], [0.63],
        [2.8], [0.83],
        [3.2], [0.85],
      )
    )
  ],
)
Although the speedup is still less than 1, we can see there is a performance gain as the average arity increases.
When the arity increases, a node has more children, and more tasks can be performed in parallel.
]

== Conclusions
- Parallelisation at the pixel-rendering level is effective
- Parallelisation at expression-tree-evaluation level presents limitations due to task synchronization overhead
- Increasing tree depth or function arity reaches better performance for expression-tree-evaluation level parallelism
