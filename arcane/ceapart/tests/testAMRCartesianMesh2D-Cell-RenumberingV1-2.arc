<?xml version="1.0"?>
<cas codename="ArcaneTest" xml:lang="fr" codeversion="1.0">
 <arcane>
  <titre>Test CartesianMesh 2D Cell Renumbering V1 (Variant 2)</titre>

  <description>Test du raffinement d'un maillage cartesian 2D avec le type d'AMR Cell et la renumerotation V1</description>

  <boucle-en-temps>AMRCartesianMeshTestLoop</boucle-en-temps>

  <modules>
    <module name="ArcanePostProcessing" active="true" />
    <module name="ArcaneCheckpoint" active="true" />
  </modules>

 </arcane>

 <arcane-post-traitement>
   <periode-sortie>1</periode-sortie>
   <depouillement>
    <variable>Density</variable>
    <variable>NodeDensity</variable>
    <groupe>AllCells</groupe>
    <groupe>AllNodes</groupe>
    <groupe>AllFacesDirection0</groupe>
    <groupe>AllFacesDirection1</groupe>
   </depouillement>
 </arcane-post-traitement>
 
 
 <maillage amr="true">
   <meshgenerator>
     <cartesian>
       <nsd>2 2</nsd>
       <origine>0.0 0.0</origine>
       <lx nx='2'>4.0</lx>
       <ly ny='2'>4.0</ly>
     </cartesian>
   </meshgenerator>
 </maillage>

 <a-m-r-cartesian-mesh-tester>
   <renumber-patch-method>1</renumber-patch-method>
   <refinement-2d>
     <position>2.0 0.0</position>
     <length>2.0 2.0</length>
   </refinement-2d>
    <refinement-2d>
     <position>0.0 0.0</position>
     <length>2.0 2.0</length>
   </refinement-2d>
   <expected-number-of-cells-in-patchs>4 4 4</expected-number-of-cells-in-patchs>
   <nodes-uid-hash>bc65bf43bfd497ef30d0c8bf27902c3e</nodes-uid-hash>
   <faces-uid-hash>28f3c42e101ca63375a3d3c35cb69e46</faces-uid-hash>
   <cells-uid-hash>caafd69302d60b35b327cfe0bc7d90af</cells-uid-hash>
 </a-m-r-cartesian-mesh-tester>

 <arcane-protections-reprises>
   <service-protection name="ArcaneBasic2CheckpointWriter" />
 </arcane-protections-reprises>
</cas>
