#ifndef SIMULATIONREPORTSTYLE_H
#define SIMULATIONREPORTSTYLE_H

namespace SimulationReportStyle
{

constexpr double PageWidthMm = 210.0;
constexpr double PageHeightMm = 297.0;

constexpr double MarginLeftMm = 30.0;
constexpr double MarginRightMm = 20.0;
constexpr double MarginTopMm = 30.0;
constexpr double MarginBottomMm = 25.0;

constexpr double HeaderMm = 25.0;
constexpr double FooterMm = 18.0;

constexpr double CoverTitlePt = 32.0;
constexpr double CoverSubtitlePt = 26.0;
constexpr double CoverInfoPt = 16.0;
constexpr double CoverDatePt = 18.0;

constexpr double Heading1Pt = 15.0;
constexpr double Heading2Pt = 12.0;
constexpr double Heading3Pt = 12.0;

constexpr double BodyPt = 12.0;
constexpr double TablePt = 10.5;
constexpr double CaptionPt = 10.5;
constexpr double HeaderPt = 12.0;
constexpr double FooterPt = 10.5;

constexpr double FigureMaxWidthMm = 150.0;
constexpr double FigureMaxHeightMm = 180.0;

constexpr double TableWidthPct = 0.86;

constexpr double TableColumn1Pct = 0.34;
constexpr double TableColumn2Pct = 0.42;
constexpr double TableColumn3Pct = 0.24;

constexpr double TableRulePt = 1.5;

constexpr double TableRowMinPt = 38.10;
constexpr double TableHeaderRowMinPt = 38.65;

constexpr double TableCellPaddingXMm = 1.9;
constexpr double TableAfterGapMm = 3.0;

constexpr double HeadingLineSpacingFactor = 1.5;
constexpr double Heading1BeforePt = 2.5;
constexpr double Heading1AfterPt = 2.5;

constexpr double HeaderLetterSpacingPt = 1.0;
constexpr double HeaderRulePt = 0.75;

constexpr double CoverDateBottomMm = 60.0;

// Cover: page top margin + this spacer ≈ PDF title at 70 mm.
constexpr double CoverTopSpacerMm = 40.0;
constexpr double CoverAfterTitleMm = 14.0;
constexpr double CoverAfterSubtitleMm = 18.0;
constexpr double CoverBeforeInfoMm = 10.0;
constexpr double CoverInfoGapMm = 6.0;
constexpr double CoverBeforeDateMm = 18.0;

} // namespace SimulationReportStyle

#endif
