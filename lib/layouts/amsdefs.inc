# Author : David L. Johnson <dlj0@lehigh.edu>
# Probably broken by Jean-Marc Lasgouttes <Jean-Marc.Lasgouttes@inria.fr>
# modified and modularized by Emmanuel GUREGHIAN <gureghia@boston.bertin.fr>
# Tinkered with Sep. '07 by Paul Rubin <rubin@msu.edu>

# Including the maths stuff
Format 4
Input amsmaths.inc

# The AMS documentclasses use the package amsmath and provide the
# functionality of makeidx.sty.
Provides amsmath         1
Provides makeidx         1


# the environments copied from the old amsart.layout are:
# - Proof
# - Bibliography
# - Title
# - Author
# - Date
# - Abstract (This is a title component for this style.)
# - Address
# - Email
# - Keywords
# - Subjectclass


Style Proof
	Margin                First_Dynamic
	LatexType             Environment
	LatexName             proof
	NextNoIndent          1
	OptionalArgs          1
	LabelSep              xx
	ParIndent             MMM
	ParSkip               0.4
	ItemSep               0.2
	TopSep                0.7
	BottomSep             0.7
	ParSep                0.3
	Align                 Block
	AlignPossible         Block, Left
	LabelType             Static
	LabelString           "Proof."
	EndLabelType          Box
	Font
	  Shape               Up
	  Size                Normal
	EndFont
	LabelFont
	  Shape               Italic
	EndFont
	# We don't want the preamble from Theorem
	Preamble
	EndPreamble
End


### Now the title stuff. We copy do not use stdstruct.inc to keep
### things in the right order
Style Title
	Margin                Static
	LatexType             Command
	InTitle               1
	LatexName             title
	ParSkip               0.4
	ItemSep               0
	TopSep                0
	BottomSep             1
	ParSep                1
	Align                 Center
	LabelType             No_Label
	OptionalArgs          1
	Font
	  Shape               Smallcaps
	  Series              Bold
	  Size                Larger
	EndFont
End


Style Author
	Margin                Static
	LatexType             Command
	InTitle               1
	LatexName             author
	LabelSep              xxx
	ParSkip               0.4
	TopSep                1.3
	BottomSep             0.7
	ParSep                0.7
	Align                 Center
	LabelType             No_Label
	Font
	  Size                Large
	  Shape               Smallcaps
	EndFont
End


Style Date
	Margin                Dynamic
	LatexType             Command
	InTitle               1
	LatexName             date
	LabelSep              xxx
	ParSkip               0.4
	TopSep                0.9
	BottomSep             0.5
	ParSep                1
	Align                 Center
	LabelType             Static
	LabelString           "Date:"
	Font
	  Size                Large
	EndFont
	LabelFont
	  Shape               Italic
	  Size                Large
	EndFont
End


Style Abstract
	Margin                First_Dynamic
	LatexType             Environment
	InTitle               1
	LatexName             abstract
	LeftMargin            MMM
	LabelIndent           MMM
	RightMargin           MMM
	ParIndent             MM
	TopSep                0.7
	BottomSep             0.7
	Align                 Block
	LabelType             Static
	LabelString           "Abstract."
	LabelSep              M
	Font
	  Size                Small
	EndFont
	LabelFont
	  Shape               Smallcaps
	EndFont
End

# Clear Address definition from lyxmacros
NoStyle Address

Style Address
	Margin                Dynamic
	LatexType             Command
	InTitle               1
	LatexName             address
	ParSkip               0.4
	BottomSep             1.5
	ParSep                1.5
	Align                 Left
	AlignPossible         Block, Left, Right, Center
	LabelType             Static
	LabelString           "Address:"
	LabelSep              M
	LabelFont
	  Shape               Italic
	EndFont
End


Style Current_Address
	CopyStyle             Address
	LatexName             curraddr
	LabelString           "Current address:"
End


Style Email
	CopyStyle             Address
	Margin                First_Dynamic
	LatexName             email
	LabelString           "E-mail address:"
End


Style URL
	CopyStyle             Address
	Margin                First_Dynamic
	LatexName             urladdr
	LabelString           "URL:"
End


Style Keywords
	CopyStyle             Address
	LatexName             keywords
	LabelString           "Key words and phrases:"
End


Style Thanks
	CopyStyle             Address
	LatexName             thanks
	LabelString           "Thanks:"
End


Style Dedicatory
	CopyStyle             Address
	LatexName             dedicatory
	LabelString           "Dedication:"
End


Style Translator
	CopyStyle             Address
	LatexName             translator
	LabelString           "Translator:"
End


Style Subjectclass
	CopyStyle             Address
	LatexName             subjclass[2000]
	LabelString           "2000 Mathematics Subject Classification:"
End


Style Bibliography
	Margin                First_Dynamic
	LatexType             Bib_Environment
	LatexName             thebibliography
	NextNoIndent          1
	LeftMargin            MM
	ParSkip               0.4
	ItemSep               0
	TopSep                0.7
	BottomSep             0.7
	ParSep                0.5
	Align                 Block
	AlignPossible         Block, Left
	LabelType             Bibliography
	LabelString           "References"
	LabelBottomSep        0.7
	Font
	  Size                Small
	EndFont
	LabelFont
	  Size                Normal
	  Shape               Smallcaps
	EndFont
End