#let conf(
  title: none,
  authors: (),
  abstract: [],
  doc,
) = {
  // Set and show rules from before.
  set page(
    paper: "us-letter",
    header: context { 
      if counter(page).get().first() > 1 [
        #align(
          right + horizon,
          title
        )
    ]
    },
    columns: 2,
    numbering: "1",
  )
  set heading(numbering: "1.1.1 ")
  set math.equation(numbering: "(1)")

  set par(justify: true)
  set text(
    font: "Libertinus Serif",
    size: 11pt,
  )

  place(
    top,
    scope: "parent",
    float: true,
    [
      #set align(center)
      #text(17pt, title)
      #let count = authors.len()
      #let ncols = calc.min(count, 3)
      #grid(
        columns: (1fr,) * ncols,
        row-gutter: 24pt,
        ..authors.map(author => [
          #author.name \
          #author.affiliation \
          #link("mailto:" + author.email)
        ]),
      )

      #par(justify: true)[
        #set align(center)
        *Abstract* \
        #set align(left)
        #abstract
      ]
    ]
  )



  set align(left)
  doc

  bibliography("ref.bib", title: "Renferences")
}

